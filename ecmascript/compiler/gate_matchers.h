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
#ifndef ECMASCRIPT_COMPILER_GATE_MATCHERS_H
#define ECMASCRIPT_COMPILER_GATE_MATCHERS_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "utils/bit_utils.h"


namespace panda::ecmascript::kungfu {

// Checks if value is in range [lowerLimit, higherLimit] using a single
// branch.
template <typename T, typename U> inline constexpr bool IsValueInRange(T value, U lowerLimit, U higherLimit)
{
    static_assert(sizeof(U) <= sizeof(T));
    using unsigned_T = typename std::make_unsigned<T>::type;
    // Use static_cast to support enum classes.
    return static_cast<unsigned_T>(static_cast<unsigned_T>(value) - static_cast<unsigned_T>(lowerLimit)) <=
           static_cast<unsigned_T>(static_cast<unsigned_T>(higherLimit) - static_cast<unsigned_T>(lowerLimit));
}

class GateMatcher {
public:
    explicit GateMatcher(GateRef gate, Circuit *circuit) : acc_(circuit), gate_(gate)
    {
    }

    GateRef Gate() const
    {
        return gate_;
    }

    GateRef ValueIn(uint32_t index)
    {
        return acc_.GetValueIn(gate_, index);
    }

    OpCode Opcode() const
    {
        return acc_.GetOpCode(gate_);
    }

    MachineType MachineType() const
    {
        return acc_.GetMachineType(gate_);
    }
    GateRef InputAt(int index) const
    {
        return acc_.GetValueIn(gate_, index);
    }

#define DECLARE_IS_GATE(NAME, OP, R, S, D, V)                                                                          \
    bool Is##NAME() const                                                                                              \
    {                                                                                                                  \
        return (Opcode() == (OpCode::OP));                                                                             \
    }
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_IS_GATE)
    GATE_META_DATA_LIST_WITH_BOOL(DECLARE_IS_GATE)
    GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(DECLARE_IS_GATE)
    GATE_META_DATA_LIST_WITH_SIZE(DECLARE_IS_GATE)
    GATE_META_DATA_LIST_WITH_ONE_PARAMETER(DECLARE_IS_GATE) GATE_META_DATA_LIST_WITH_PC_OFFSET(DECLARE_IS_GATE)
        GATE_META_DATA_LIST_FOR_CALL(DECLARE_IS_GATE) GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(DECLARE_IS_GATE)
        GATE_META_DATA_LIST_FOR_NEW(DECLARE_IS_GATE)
#undef DECLARE_IS_GATE

#define DECLARE_IS_INST(NAME, OPCODEID, MACHINETYPEID)                                                                 \
    bool Ism##NAME() const                                                                                             \
    {                                                                                                                  \
        return Is##OPCODEID() && MachineType() == (MACHINETYPEID);                                                     \
    }
            BINARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(DECLARE_IS_INST)
                UNARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(DECLARE_IS_INST)
#undef DECLARE_IS_INST

#define DECLARE_IS_INST(NAME, OPCODEID, CONDITION)                                                                     \
    bool Ism##NAME() const                                                                                             \
    {                                                                                                                  \
        return Is##OPCODEID() && static_cast<uint64_t>(CONDITION) == ConditionValue();                                 \
    };                                                                                                                 \
    BINARY_CMP_METHOD_LIST_WITHOUT_BITWIDTH(DECLARE_IS_INST)
#undef DECLARE_IS_INST

    bool Equals(const GateRef gate)
    {
        return gate == Gate();
    }

    GateAccessor acc_;

    BitField ConditionValue()
    {
        ASSERT(IsFcmp() || IsIcmp());
        return acc_.TryGetValue(Gate());
    }
private:
    GateRef gate_;
};

// A pattern matcher for abitrary value constants.
//
// Note that value identities on the input gate are skipped when matching. The
// resolved value may not be a parameter of the input gate. The Gate() method
// returns the unmodified input gate. This is by design, as reducers may wish to
// match value constants but delay reducing the gate until a later phase.
template <typename T, OpCode kOpcode, MachineType kMachineType> struct ValueMatcher : public GateMatcher {
    using ValueType = T;

    explicit ValueMatcher(GateRef gate, Circuit *circuit)
        : GateMatcher(gate, circuit), resolvedValue_(), hasResolvedValue_(false)
    {
        if (acc_.GetOpCode(gate) == kOpcode && acc_.GetMachineType(gate) == kMachineType) {
            hasResolvedValue_ = true;
        }

        if (hasResolvedValue_) {
            if (kMachineType == F64) {
                resolvedValue_ = acc_.GetFloat64FromConstant(gate);
            } else if (kMachineType == I32) {
                resolvedValue_ = acc_.GetInt32FromConstant(gate);
            } else if (kMachineType == I64) {
                resolvedValue_ = acc_.GetConstantValue(gate);
            } else {
                hasResolvedValue_ = false;
            }
        }
    }

    bool HasResolvedValue() const
    {
        return hasResolvedValue_;
    }
    const T &ResolvedValue() const
    {
        ASSERT(HasResolvedValue());
        return resolvedValue_;
    }

private:
    T resolvedValue_;
    bool hasResolvedValue_;
};

// A pattern matcher for integer constants.
template <typename T, OpCode kOpcode, MachineType kMachineType>
struct IntMatcher final : public ValueMatcher<T, kOpcode, kMachineType> {
    explicit IntMatcher(GateRef gate, Circuit *circuit) : ValueMatcher<T, kOpcode, kMachineType>(gate, circuit)
    {
    }

    bool Is(const T &value) const
    {
        return this->HasResolvedValue() && this->ResolvedValue() == value;
    }

    bool IsInRange(const T &low, const T &high) const
    {
        return this->HasResolvedValue() && IsValueInRange(this->ResolvedValue(), low, high);
    }
    bool IsMultipleOf(T n) const
    {
        if (!this->HasResolvedValue()) {
            return false;
        }
        return (this->ResolvedValue() % n) == 0;
    }

    bool IsPowerOf2() const
    {
        if (!this->HasResolvedValue() || this->ResolvedValue() <= 0) {
            return false;
        }
        using unsigned_type = typename std::make_unsigned<T>::type;
        const unsigned_type resolvedValue = static_cast<unsigned_type>(this->ResolvedValue());
        return (resolvedValue & (resolvedValue - 1)) == 0;
    }

    bool IsNegativePowerOf2() const
    {
        if (!this->HasResolvedValue() || this->ResolvedValue() >= 0) {
            return false;
        }
        return ((this->ResolvedValue() == std::numeric_limits<T>::min()) ||
                (-this->ResolvedValue() & (-this->ResolvedValue() - 1)) == 0);
    }

    bool IsNegative() const
    {
        if (!this->HasResolvedValue()) {
            return false;
        }
        return this->ResolvedValue() < 0;
    }
};

using Int32Matcher = IntMatcher<int32_t, OpCode::CONSTANT, MachineType::I32>;
using Uint32Matcher = IntMatcher<uint32_t, OpCode::CONSTANT, MachineType::I32>;
using Int64Matcher = IntMatcher<int64_t, OpCode::CONSTANT, MachineType::I64>;
using Uint64Matcher = IntMatcher<uint64_t, OpCode::CONSTANT, MachineType::I64>;


// A pattern matcher for floating point constants.
template <typename T, OpCode kOpcode, MachineType kMachineType>
struct FloatMatcher final : public ValueMatcher<T, kOpcode, kMachineType> {
    explicit FloatMatcher(GateRef gate, Circuit *circuit) : ValueMatcher<T, kOpcode, kMachineType>(gate, circuit)
    {
    }

    bool Is(const T &value) const
    {
        return this->HasResolvedValue() && this->ResolvedValue() == value;
    }
    bool IsInRange(const T &low, const T &high) const
    {
        return this->HasResolvedValue() && low <= this->ResolvedValue() && this->ResolvedValue() <= high;
    }
    bool IsMinusZero() const
    {
        if (!this->HasResolvedValue()) {
            return false;
        }
        return this->Is(0.0) && std::signbit(this->ResolvedValue());
    }

    bool IsNegative() const
    {
        if (!this->HasResolvedValue()) {
            return false;
        }
        return this->ResolvedValue() < 0.0;
    }

    bool IsNaN() const
    {
        if (!this->HasResolvedValue()) {
            return false;
        }
        return std::isnan(this->ResolvedValue());
    }

    bool IsZero() const
    {
        if (!this->HasResolvedValue()) {
            return false;
        }
        return this->Is(0.0) && !std::signbit(this->ResolvedValue());
    }

    bool IsNormal() const
    {
        if (!this->HasResolvedValue()) {
            return false;
        }
        return std::isnormal(this->ResolvedValue());
    }

    bool IsInteger() const
    {
        if (!this->HasResolvedValue()) {
            return false;
        }
        return std::nearbyint(this->ResolvedValue()) == this->ResolvedValue();
    }
};

using Float32Matcher = FloatMatcher<float, OpCode::CONSTANT, MachineType::F32>;
using Float64Matcher = FloatMatcher<double, OpCode::CONSTANT, MachineType::F64>;


// For shorter pattern matching code, this struct matches both the left and
// right hand sides of a binary operation and can put constants on the right
// if they appear on the left hand side of a commutative operation.
template <typename LeftExpr, typename RightExpr, MachineType rep> struct BinopMatcher : public GateMatcher {
    explicit BinopMatcher(GateRef gate, Circuit *circuit)
        : GateMatcher(gate, circuit), left_(InputAt(0), circuit), right_(InputAt(1), circuit)
    {
        if (IsCommutative(Opcode())) {
            PutConstantOnRight();
        }
    }

    BinopMatcher(GateRef gate, bool allowInputSwap, Circuit *circuit)
        : GateMatcher(gate, circuit), left_(InputAt(0), circuit), right_(InputAt(1), circuit)
    {
        if (allowInputSwap) {
            PutConstantOnRight();
        }
    }

    using LeftMatcher = LeftExpr;
    using RightMatcher = RightExpr;

    // static constexpr MachineType representation = rep;

    const LeftExpr &Left() const
    {
        return left_;
    }

    const RightExpr &Right() const
    {
        return right_;
    }

    void SetLeft(GateRef left, Circuit* circuit)
    {
        left_  = LeftExpr(left, circuit);
    }

    void SetRight(GateRef right, Circuit* circuit)
    {
        right_ = RightExpr(right, circuit);
    }

    bool IsFoldable() const
    {
        return Left().HasResolvedValue() && Right().HasResolvedValue();
    }
    bool LeftEqualsRight() const
    {
        return Left().Gate() == Right().Gate();
    }

    bool OwnsInput(GateRef input)
    {
        auto use = acc_.Uses(input);
        for (auto it = use.begin(); it != use.end(); it++) {
            if (*it != Gate()) {
                return false;
            }
        }
        return true;
    }

protected:
    void SwapInputs()
    {
        std::swap(left_, right_);
        acc_.ReplaceValueIn(Gate(), Left().Gate(), 0);
        acc_.ReplaceValueIn(Gate(), Right().Gate(), 1);
    }

private:
    void PutConstantOnRight()
    {
        if (Left().HasResolvedValue() && !Right().HasResolvedValue()) {
            SwapInputs();
        }
    }
    static bool IsCommutative(OpCode opcode)
    {
        switch (opcode) {
            case OpCode::ADD:
            case OpCode::AND:
            case OpCode::OR:
            case OpCode::XOR:
            case OpCode::MUL:
            case OpCode::ADD_WITH_OVERFLOW:
            case OpCode::MUL_WITH_OVERFLOW:
                return true;
            default:
                return false;
        }
        return false;
    }
    LeftExpr left_;
    RightExpr right_;
};

using Int32BinopMatcher = BinopMatcher<Int32Matcher, Int32Matcher, MachineType::I32>;
using Uint32BinopMatcher = BinopMatcher<Uint32Matcher, Uint32Matcher, MachineType::I32>;
using Int64BinopMatcher = BinopMatcher<Int64Matcher, Int64Matcher, MachineType::I64>;
using Uint64BinopMatcher = BinopMatcher<Uint64Matcher, Uint64Matcher, MachineType::I64>;
using Float32BinopMatcher = BinopMatcher<Float32Matcher, Float32Matcher, MachineType::F32>;
using Float64BinopMatcher = BinopMatcher<Float64Matcher, Float64Matcher, MachineType::F64>;


} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_GATE_MATCHERS_H