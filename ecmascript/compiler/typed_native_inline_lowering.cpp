/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/typed_native_inline_lowering.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

namespace panda::ecmascript::kungfu {
GateRef TypedNativeInlineLowering::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::MATH_COS:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatCos));
            break;
        case OpCode::MATH_COSH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatCosh));
            break;
        case OpCode::MATH_SIN:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatSin));
            break;
        case OpCode::MATH_LOG:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatLog));
            break;
        case OpCode::MATH_LOG2:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatLog2));
            break;
        case OpCode::MATH_LOG10:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatLog10));
            break;
        case OpCode::MATH_LOG1P:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatLog1p));
            break;
        case OpCode::MATH_SINH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatSinh));
            break;
        case OpCode::MATH_ASINH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatAsinh));
            break;
        case OpCode::MATH_TAN:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatTan));
            break;
        case OpCode::MATH_ATAN:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatAtan));
            break;
        case OpCode::MATH_TANH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatTanh));
            break;
        case OpCode::MATH_ACOS:
            LowerGeneralUnaryMath<MathTrigonometricCheck::ABS_GT_ONE>(gate, RTSTUB_ID(FloatAcos));
            break;
        case OpCode::MATH_ASIN:
            LowerGeneralUnaryMath<MathTrigonometricCheck::ABS_GT_ONE>(gate, RTSTUB_ID(FloatAsin));
            break;
        case OpCode::MATH_ATANH:
            LowerGeneralUnaryMath<MathTrigonometricCheck::ABS_GT_ONE>(gate, RTSTUB_ID(FloatAtanh));
            break;
        case OpCode::MATH_ACOSH:
            LowerGeneralUnaryMath<MathTrigonometricCheck::LT_ONE>(gate, RTSTUB_ID(FloatAcosh));
            break;
        case OpCode::MATH_ATAN2:
            LowerMathAtan2(gate);
            break;
        case OpCode::MATH_ABS:
            LowerAbs(gate);
            break;
        case OpCode::MATH_POW:
            LowerMathPow(gate);
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

void TypedNativeInlineLowering::LowerMathPow(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef base = acc_.GetValueIn(gate, 0);
    GateRef exp = acc_.GetValueIn(gate, 1);

    Label exit(&builder_);
    Label notNan(&builder_);

    auto nanValue = builder_.NanValue();
    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), nanValue);

    const double doubleOne = 1.0;
    GateRef baseIsOne = builder_.DoubleEqual(base, builder_.Double(doubleOne));
    // Base is 1.0, exponent is inf => NaN
    // Exponent is not finit, if is NaN or is Inf
    GateRef tempIsNan = builder_.BoolAnd(baseIsOne, builder_.DoubleIsINF(exp));
    GateRef resultIsNan = builder_.BoolOr(builder_.DoubleIsNAN(exp), tempIsNan);

    BRANCH_CIR(resultIsNan, &exit, &notNan);
    builder_.Bind(&notNan);
    {
        GateRef glue = acc_.GetGlueFromArgList();
        result = builder_.CallNGCRuntime(glue, RTSTUB_ID(FloatPow), Gate::InvalidGateRef, {base, exp}, gate);
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

template <TypedNativeInlineLowering::MathTrigonometricCheck CHECK>
void TypedNativeInlineLowering::LowerGeneralUnaryMath(GateRef gate, RuntimeStubCSigns::ID stubId)
{
    Environment env(gate, circuit_, &builder_);

    Label exit(&builder_);
    Label checkNotPassed(&builder_);

    GateRef value = acc_.GetValueIn(gate, 0);
    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), builder_.Double(base::NAN_VALUE));

    GateRef check;
    const double doubleOne = 1.0;
    if constexpr (CHECK == TypedNativeInlineLowering::MathTrigonometricCheck::NOT_NAN) {
        check = builder_.DoubleIsNAN(value);
    } else if constexpr (CHECK == TypedNativeInlineLowering::MathTrigonometricCheck::LT_ONE) {
        check = builder_.DoubleLessThan(value, builder_.Double(doubleOne));
    } else if constexpr (CHECK == TypedNativeInlineLowering::MathTrigonometricCheck::ABS_GT_ONE) {
        auto gt = builder_.DoubleGreaterThan(value, builder_.Double(doubleOne));
        auto lt = builder_.DoubleLessThan(value, builder_.Double(-doubleOne));
        check = builder_.BoolOr(gt, lt);
    }

    BRANCH_CIR(check, &exit, &checkNotPassed);
    builder_.Bind(&checkNotPassed);
    {
        GateRef glue = acc_.GetGlueFromArgList();
        result = builder_.CallNGCRuntime(glue, stubId, Gate::InvalidGateRef, {value}, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerMathAtan2(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef y = acc_.GetValueIn(gate, 0);
    GateRef x = acc_.GetValueIn(gate, 1);

    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), builder_.Double(base::NAN_VALUE));

    Label exit(&builder_);
    Label label1(&builder_);
    Label label2(&builder_);
    Label label3(&builder_);

    auto yIsNan = builder_.DoubleIsNAN(y);
    auto xIsNan = builder_.DoubleIsNAN(x);
    auto checkNaN = builder_.BoolOr(yIsNan, xIsNan);
    BRANCH_CIR(checkNaN, &exit, &label1);
    builder_.Bind(&label1);
    {
        Label label4(&builder_);
        auto yIsZero = builder_.DoubleEqual(y, builder_.Double(0.));
        auto xIsMoreZero = builder_.DoubleGreaterThan(x, builder_.Double(0.));
        auto check = builder_.BoolAnd(yIsZero, xIsMoreZero);
        BRANCH_CIR(check, &label4, &label2);
        builder_.Bind(&label4);
        {
            result = y;
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&label2);
    {
        Label label5(&builder_);
        auto xIsPositiveInf = builder_.DoubleEqual(x, builder_.Double(std::numeric_limits<double>::infinity()));
        BRANCH_CIR(xIsPositiveInf, &label5, &label3);
        builder_.Bind(&label5);
        {
            Label label6(&builder_);
            Label label7(&builder_);
            auto yPositiveCheck = builder_.DoubleGreaterThanOrEqual(y, builder_.Double(0.));
            BRANCH_CIR(yPositiveCheck, &label6, &label7);
            builder_.Bind(&label6);
            {
                result = builder_.Double(0.0);
                builder_.Jump(&exit);
            }
            builder_.Bind(&label7);
            {
                result = builder_.Double(-0.0);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&label3);
    {
        GateRef glue = acc_.GetGlueFromArgList();
        result = builder_.CallNGCRuntime(glue, RTSTUB_ID(FloatAtan2), Gate::InvalidGateRef, {y, x}, gate);
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

//  Int abs : The internal representation of an integer is inverse code,
//  The absolute value of a negative number can be found by inverting it by adding one.

//  Float abs : A floating-point number is composed of mantissa and exponent.
//  The length of mantissa will affect the precision of the number, and its sign will determine the sign of the number.
//  The absolute value of a floating-point number can be found by setting mantissa sign bit to 0.
void TypedNativeInlineLowering::LowerAbs(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label exit(&builder_);
    GateRef param = acc_.GetValueIn(gate, 0);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());

    Label isInt(&builder_);
    Label notInt(&builder_);
    Label isIntMin(&builder_);
    Label isResultInt(&builder_);
    Label intExit(&builder_);
    BRANCH_CIR(builder_.TaggedIsInt(param), &isInt, &notInt);
    builder_.Bind(&isInt);
    {
        auto value = builder_.GetInt32OfTInt(param);
        BRANCH_CIR(builder_.Equal(value, builder_.Int32(INT32_MIN)), &isIntMin, &isResultInt);
        builder_.Bind(&isResultInt);
        {
            auto temp = builder_.Int32ASR(value, builder_.Int32(JSTaggedValue::INT_SIGN_BIT_OFFSET));
            auto res = builder_.Int32Xor(value, temp);
            result = builder_.Int32ToTaggedPtr(builder_.Int32Sub(res, temp));
            builder_.Jump(&intExit);
        }
        builder_.Bind(&isIntMin);
        {
            result = builder_.DoubleToTaggedDoublePtr(builder_.Double(-static_cast<double>(INT_MIN)));
            builder_.Jump(&intExit);
        }
        // Aot compiler fails without jump to intermediate label
        builder_.Bind(&intExit);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notInt);
    {
        auto value = builder_.GetDoubleOfTDouble(param);
        // set the sign bit to 0 by shift left then right.
        auto temp = builder_.Int64LSL(builder_.CastDoubleToInt64(value), builder_.Int64(1));
        auto res = builder_.Int64LSR(temp, builder_.Int64(1));
        result = builder_.DoubleToTaggedDoublePtr(builder_.CastInt64ToFloat64(res));
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

}

