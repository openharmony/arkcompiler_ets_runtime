/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_constant_folding.h"

#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/base/math_helper.h"
#include "ecmascript/base/number_helper.h"

#include <cmath>
#include <limits>

namespace panda::ecmascript::arksteed {
namespace {

struct PrimitiveConstant {
    JSTaggedValue value;

    static bool FromVertex(ValueVertex *vertex, PrimitiveConstant *constant)
    {
        if (auto *tagged = vertex->TryCast<TaggedConstantVertex>()) {
            return FromTagged(JSTaggedValue(tagged->GetValue()), constant);
        }
        return false;
    }

    bool IsInt32(int32_t *out) const
    {
        if (!value.IsInt()) {
            return false;
        }
        *out = value.GetInt();
        return true;
    }

    bool IsNumber() const
    {
        return value.IsInt() || value.IsDouble();
    }

    double NumberValue() const
    {
        ASSERT(IsNumber());
        return value.IsInt() ? static_cast<double>(value.GetInt()) : value.GetDouble();
    }

    bool IsNullish() const
    {
        return value.IsNull() || value.IsUndefined();
    }

    bool IsBoolean() const
    {
        return value.IsTrue() || value.IsFalse();
    }

    PrimitiveConstant ToNumberConstant() const
    {
        ASSERT(IsBoolean());
        return {JSTaggedValue(value.IsTrue() ? 1 : 0)};
    }

    bool CanFoldEquality() const
    {
        return IsNumber() || IsBoolean() || IsNullish();
    }

    bool CanFoldToNumber() const
    {
        return IsNumber() || IsBoolean() || value.IsNull() || value.IsUndefined();
    }

private:
    static bool FromTagged(JSTaggedValue value, PrimitiveConstant *constant)
    {
        if (!value.IsInt() && !value.IsDouble() && !value.IsTrue() && !value.IsFalse() && !value.IsUndefined() &&
            !value.IsNull() && !value.IsHole()) {
            return false;
        }
        constant->value = value;
        return true;
    }
};

bool TryMakeTaggedInt32Constant(int64_t value, JSTaggedValue *result)
{
    if (value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max()) {
        return false;
    }
    *result = JSTaggedValue(static_cast<int32_t>(value));
    return true;
}

bool TryMakeTaggedDoubleConstant(double value, JSTaggedValue *result)
{
    if (std::isnan(value)) {
        *result = JSTaggedValue(base::NumberHelper::GetNaN());
        return true;
    }
    if (JSTaggedValue::IsImpureNaN(value)) {
        return false;
    }
    *result = JSTaggedValue(value);
    return true;
}

bool TryFoldInt32Arithmetic(BinaryFoldOp op, int32_t left, int32_t right, JSTaggedValue *result)
{
    int32_t folded = 0;
    switch (op) {
        case BinaryFoldOp::ADD:
            if (base::SignedAddOverflow32(left, right, &folded)) {
                return false;
            }
            *result = JSTaggedValue(folded);
            return true;
        case BinaryFoldOp::SUB:
            if (base::SignedSubOverflow32(left, right, &folded)) {
                return false;
            }
            *result = JSTaggedValue(folded);
            return true;
        case BinaryFoldOp::MUL:
            if (base::SignedMulOverflow32(left, right, &folded)) {
                return false;
            }
            if (folded == 0 && (left < 0 || right < 0)) {
                return false;
            }
            *result = JSTaggedValue(folded);
            return true;
        default:
            return false;
    }
}

bool TryFoldInt32Bitwise(BinaryFoldOp op, int32_t left, int32_t right, JSTaggedValue *result)
{
    uint32_t shift = static_cast<uint32_t>(right) & 0x1FU;
    switch (op) {
        case BinaryFoldOp::SHL: {
            *result = JSTaggedValue(static_cast<int32_t>(static_cast<uint32_t>(left) << shift));
            return true;
        }
        case BinaryFoldOp::SHR: {
            *result = JSTaggedValue(static_cast<uint32_t>(left) >> shift);
            return true;
        }
        case BinaryFoldOp::ASHR:
            *result = JSTaggedValue(left >> shift);
            return true;
        case BinaryFoldOp::AND:
            *result = JSTaggedValue(left & right);
            return true;
        case BinaryFoldOp::OR:
            *result = JSTaggedValue(left | right);
            return true;
        case BinaryFoldOp::XOR:
            *result = JSTaggedValue(left ^ right);
            return true;
        default:
            return false;
    }
}

bool TryFoldInt32Binary(BinaryFoldOp op, int32_t left, int32_t right, JSTaggedValue *result)
{
    return TryFoldInt32Arithmetic(op, left, right, result) || TryFoldInt32Bitwise(op, left, right, result);
}

bool TryFoldInt32Compare(BinaryFoldOp op, int32_t left, int32_t right, JSTaggedValue *result)
{
    switch (op) {
        case BinaryFoldOp::LESS:
            *result = JSTaggedValue(left < right);
            return true;
        case BinaryFoldOp::LESS_EQ:
            *result = JSTaggedValue(left <= right);
            return true;
        case BinaryFoldOp::GREATER:
            *result = JSTaggedValue(left > right);
            return true;
        case BinaryFoldOp::GREATER_EQ:
            *result = JSTaggedValue(left >= right);
            return true;
        default:
            return false;
    }
}

bool TryFoldFloat64Divide(double left, double right, JSTaggedValue *result)
{
    if (right == 0) {
        if (left == 0 || std::isnan(left)) {
            *result = JSTaggedValue(base::NumberHelper::GetNaN());
            return true;
        }
        double quotient = std::numeric_limits<double>::infinity();
        if (std::signbit(left) != std::signbit(right)) {
            quotient = -quotient;
        }
        return TryMakeTaggedDoubleConstant(quotient, result);
    }
    return TryMakeTaggedDoubleConstant(left / right, result);
}

bool TryFoldFloat64Arithmetic(BinaryFoldOp op, double left, double right, JSTaggedValue *result)
{
    switch (op) {
        case BinaryFoldOp::ADD:
            return TryMakeTaggedDoubleConstant(left + right, result);
        case BinaryFoldOp::SUB:
            return TryMakeTaggedDoubleConstant(left - right, result);
        case BinaryFoldOp::MUL:
            return TryMakeTaggedDoubleConstant(left * right, result);
        case BinaryFoldOp::DIV:
            return TryFoldFloat64Divide(left, right, result);
        default:
            return false;
    }
}

bool TryFoldFloat64Compare(BinaryFoldOp op, double left, double right, JSTaggedValue *result)
{
    switch (op) {
        case BinaryFoldOp::LESS:
            *result = JSTaggedValue(left < right);
            return true;
        case BinaryFoldOp::LESS_EQ:
            *result = JSTaggedValue(left <= right);
            return true;
        case BinaryFoldOp::GREATER:
            *result = JSTaggedValue(left > right);
            return true;
        case BinaryFoldOp::GREATER_EQ:
            *result = JSTaggedValue(left >= right);
            return true;
        default:
            return false;
    }
}

bool TryFoldFloat64Binary(BinaryFoldOp op, PrimitiveConstant left, PrimitiveConstant right, JSTaggedValue *result)
{
    if (!left.IsNumber() || !right.IsNumber() || (!left.value.IsDouble() && !right.value.IsDouble())) {
        return false;
    }

    double leftNumber = left.NumberValue();
    double rightNumber = right.NumberValue();
    return TryFoldFloat64Arithmetic(op, leftNumber, rightNumber, result) ||
           TryFoldFloat64Compare(op, leftNumber, rightNumber, result);
}

bool TryFoldLooseEqual(PrimitiveConstant left, PrimitiveConstant right, bool *result)
{
    if (!left.CanFoldEquality() || !right.CanFoldEquality()) {
        return false;
    }
    if (left.IsNullish() || right.IsNullish()) {
        *result = left.IsNullish() && right.IsNullish();
        return true;
    }
    if (left.IsBoolean()) {
        left = left.ToNumberConstant();
    }
    if (right.IsBoolean()) {
        right = right.ToNumberConstant();
    }
    if (left.IsNumber() && right.IsNumber()) {
        double leftNumber = left.NumberValue();
        double rightNumber = right.NumberValue();
        *result = !std::isnan(leftNumber) && !std::isnan(rightNumber) && leftNumber == rightNumber;
        return true;
    }
    return false;
}

bool TryFoldStrictEqual(PrimitiveConstant left, PrimitiveConstant right, bool *result)
{
    if (!left.CanFoldEquality() || !right.CanFoldEquality()) {
        return false;
    }
    if (left.IsNumber() || right.IsNumber()) {
        if (!left.IsNumber() || !right.IsNumber()) {
            *result = false;
            return true;
        }
        double leftNumber = left.NumberValue();
        double rightNumber = right.NumberValue();
        *result = !std::isnan(leftNumber) && !std::isnan(rightNumber) && leftNumber == rightNumber;
        return true;
    }
    *result = left.value.GetRawData() == right.value.GetRawData();
    return true;
}

bool TryFoldEquality(BinaryFoldOp op, PrimitiveConstant left, PrimitiveConstant right, JSTaggedValue *result)
{
    bool folded = false;
    switch (op) {
        case BinaryFoldOp::EQ:
        case BinaryFoldOp::NOT_EQ:
            if (!TryFoldLooseEqual(left, right, &folded)) {
                return false;
            }
            *result = JSTaggedValue(op == BinaryFoldOp::EQ ? folded : !folded);
            return true;
        case BinaryFoldOp::STRICT_EQ:
        case BinaryFoldOp::STRICT_NOT_EQ:
            if (!TryFoldStrictEqual(left, right, &folded)) {
                return false;
            }
            *result = JSTaggedValue(op == BinaryFoldOp::STRICT_EQ ? folded : !folded);
            return true;
        default:
            return false;
    }
}

bool TryFoldBinary(ValueVertex *lhs, ValueVertex *rhs, BinaryFoldOp op, JSTaggedValue *result)
{
    PrimitiveConstant left;
    PrimitiveConstant right;
    if (!PrimitiveConstant::FromVertex(lhs, &left) || !PrimitiveConstant::FromVertex(rhs, &right)) {
        return false;
    }

    int32_t leftInt = 0;
    int32_t rightInt = 0;
    if (left.IsInt32(&leftInt) && right.IsInt32(&rightInt)) {
        if (TryFoldInt32Binary(op, leftInt, rightInt, result)) {
            return true;
        }
        if (TryFoldInt32Compare(op, leftInt, rightInt, result)) {
            return true;
        }
    }

    if (TryFoldFloat64Binary(op, left, right, result)) {
        return true;
    }

    return TryFoldEquality(op, left, right, result);
}

bool TryFoldInt32Unary(UnaryFoldOp op, int32_t value, JSTaggedValue *result)
{
    switch (op) {
        case UnaryFoldOp::NEG:
            if (value == 0 || value == std::numeric_limits<int32_t>::min()) {
                return false;
            }
            *result = JSTaggedValue(-value);
            return true;
        case UnaryFoldOp::INC:
            return TryMakeTaggedInt32Constant(static_cast<int64_t>(value) + 1, result);
        case UnaryFoldOp::DEC:
            return TryMakeTaggedInt32Constant(static_cast<int64_t>(value) - 1, result);
        default:
            return false;
    }
}

bool TryFoldFloat64Unary(UnaryFoldOp op, double value, JSTaggedValue *result)
{
    switch (op) {
        case UnaryFoldOp::NEG:
            return TryMakeTaggedDoubleConstant(-value, result);
        case UnaryFoldOp::INC:
            return TryMakeTaggedDoubleConstant(value + 1, result);
        case UnaryFoldOp::DEC:
            return TryMakeTaggedDoubleConstant(value - 1, result);
        default:
            return false;
    }
}

bool TryFoldNumericUnary(UnaryFoldOp op, PrimitiveConstant value, JSTaggedValue *result)
{
    int32_t intValue = 0;
    if (value.IsInt32(&intValue)) {
        return TryFoldInt32Unary(op, intValue, result);
    }
    if (value.value.IsDouble()) {
        return TryFoldFloat64Unary(op, value.NumberValue(), result);
    }
    return false;
}

bool TryGetNumberForConversion(PrimitiveConstant value, double *number)
{
    if (!value.CanFoldToNumber()) {
        return false;
    }
    if (value.IsNumber()) {
        *number = value.NumberValue();
    } else if (value.IsBoolean()) {
        *number = value.value.IsTrue() ? 1 : 0;
    } else if (value.value.IsNull()) {
        *number = 0;
    } else {
        ASSERT(value.value.IsUndefined());
        *number = std::numeric_limits<double>::quiet_NaN();
    }
    return true;
}

bool TryFoldBitwiseNot(PrimitiveConstant value, JSTaggedValue *result)
{
    double number = 0;
    if (!TryGetNumberForConversion(value, &number)) {
        return false;
    }
    int32_t converted = base::NumberHelper::DoubleToInt(number, base::INT32_BITS);
    *result = JSTaggedValue(~converted); // NOLINT(hicpp-signed-bitwise)
    return true;
}

bool TryFoldToNumberOrNumeric(PrimitiveConstant value, JSTaggedValue *result)
{
    if (!value.CanFoldToNumber()) {
        return false;
    }
    if (value.IsNumber()) {
        *result = value.value;
    } else if (value.IsBoolean()) {
        *result = JSTaggedValue(value.value.IsTrue() ? 1 : 0);
    } else if (value.value.IsNull()) {
        *result = JSTaggedValue(0);
    } else {
        ASSERT(value.value.IsUndefined());
        *result = JSTaggedValue(base::NumberHelper::GetNaN());
    }
    return true;
}

bool TryFoldUnary(ValueVertex *input, UnaryFoldOp op, JSTaggedValue *result)
{
    PrimitiveConstant value;
    if (!PrimitiveConstant::FromVertex(input, &value)) {
        return false;
    }

    switch (op) {
        case UnaryFoldOp::NEG:
        case UnaryFoldOp::INC:
        case UnaryFoldOp::DEC:
            return TryFoldNumericUnary(op, value, result);
        case UnaryFoldOp::NOT:
            return TryFoldBitwiseNot(value, result);
        case UnaryFoldOp::TO_NUMBER:
        case UnaryFoldOp::TO_NUMERIC:
            return TryFoldToNumberOrNumeric(value, result);
    }
    return false;
}
}  // namespace

bool TryFoldToBooleanConstant(ValueVertex *vertex, bool *result)
{
    PrimitiveConstant value;
    if (!PrimitiveConstant::FromVertex(vertex, &value)) {
        return false;
    }
    *result = value.value.ToBoolean();
    return true;
}

bool TryFoldBinaryConstant(ValueVertex *lhs, ValueVertex *rhs, BinaryFoldOp op, JSTaggedValue *result)
{
    return TryFoldBinary(lhs, rhs, op, result);
}

bool TryFoldUnaryConstant(ValueVertex *input, UnaryFoldOp op, JSTaggedValue *result)
{
    return TryFoldUnary(input, op, result);
}

}  // namespace panda::ecmascript::arksteed
