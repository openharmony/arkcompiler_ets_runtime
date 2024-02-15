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
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatCos));
            break;
        case OpCode::MATH_COSH:
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatCosh));
            break;
        case OpCode::MATH_SIN:
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatSin));
            break;
        case OpCode::MATH_SINH:
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatSinh));
            break;
        case OpCode::MATH_ASINH:
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatAsinh));
            break;
        case OpCode::MATH_TAN:
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatTan));
            break;
        case OpCode::MATH_ATAN:
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatAtan));
            break;
        case OpCode::MATH_TANH:
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatTanh));
            break;
        case OpCode::MATH_ACOS:
            LowerMathTrigonometric<TypedNativeInlineLowering::MathTrigonometricCheck::ABS_GT_ONE>(gate, RTSTUB_ID(FloatAcos));
            break;
        case OpCode::MATH_ASIN:
            LowerMathTrigonometric<TypedNativeInlineLowering::MathTrigonometricCheck::ABS_GT_ONE>(gate, RTSTUB_ID(FloatAsin));
            break;
        case OpCode::MATH_ATANH:
            LowerMathTrigonometric<TypedNativeInlineLowering::MathTrigonometricCheck::ABS_GT_ONE>(gate, RTSTUB_ID(FloatAtanh));
            break;
        case OpCode::MATH_ACOSH:
            LowerMathTrigonometric<TypedNativeInlineLowering::MathTrigonometricCheck::LT_ONE>(gate, RTSTUB_ID(FloatAcosh));
            break;
        case OpCode::MATH_ATAN2:
            LowerMathAtan2(gate);
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

template <TypedNativeInlineLowering::MathTrigonometricCheck CHECK>
void TypedNativeInlineLowering::LowerMathTrigonometric(GateRef gate, RuntimeStubCSigns::ID stub_id)
{
    Environment env(gate, circuit_, &builder_);
    builder_.SetEnvironment(&env);

    Label exit(&builder_);
    Label checkNotPassed(&builder_);

    GateRef param = acc_.GetValueIn(gate, 0);
    auto valueNaN = builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), valueNaN);

    GateRef value = builder_.GetDoubleOfTNumber(param);
    GateRef check;
    if constexpr (CHECK == TypedNativeInlineLowering::MathTrigonometricCheck::NOT_NAN) {
        check = builder_.DoubleIsNAN(value);
    } else if constexpr (CHECK == TypedNativeInlineLowering::MathTrigonometricCheck::LT_ONE) {
        check = builder_.DoubleLessThan(value, builder_.Double(1.));
    } else if constexpr (CHECK == TypedNativeInlineLowering::MathTrigonometricCheck::ABS_GT_ONE) {
        auto gt = builder_.DoubleGreaterThan(value, builder_.Double(1.));
        auto lt = builder_.DoubleLessThan(value, builder_.Double(-1.));
        check = builder_.BoolOr(gt, lt);
    }

    builder_.Branch(check, &exit, &checkNotPassed);
    builder_.Bind(&checkNotPassed);
    {
        GateRef glue = acc_.GetGlueFromArgList();
        result = builder_.CallNGCRuntime(glue, stub_id, Gate::InvalidGateRef, {value}, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerMathAtan2(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    builder_.SetEnvironment(&env);

    GateRef y = builder_.GetDoubleOfTNumber(acc_.GetValueIn(gate, 0));
    GateRef x = builder_.GetDoubleOfTNumber(acc_.GetValueIn(gate, 1));

    auto valueNaN = builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), valueNaN);

    Label exit(&builder_);
    Label label1(&builder_);
    Label label2(&builder_);
    Label label3(&builder_);

    auto yIsNan = builder_.DoubleIsNAN(y);
    auto xIsNan = builder_.DoubleIsNAN(x);
    auto checkNaN = builder_.BoolOr(yIsNan, xIsNan);
    builder_.Branch(checkNaN, &exit, &label1);
    builder_.Bind(&label1);
    {
        Label label4(&builder_);
        auto yIsZero = builder_.DoubleEqual(y, builder_.Double(0.));
        auto xIsMoreZero = builder_.DoubleGreaterThan(x, builder_.Double(0.));
        auto check = builder_.BoolAnd(yIsZero, xIsMoreZero);
        builder_.Branch(check, &label4, &label2);
        builder_.Bind(&label4);
        {
            result = acc_.GetValueIn(gate, 0); // return Y
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&label2);
    {
        Label label5(&builder_);
        auto xIsPositiveInf = builder_.DoubleEqual(x, builder_.Double(std::numeric_limits<double>::infinity()));
        builder_.Branch(xIsPositiveInf, &label5, &label3);
        builder_.Bind(&label5);
        {
            Label label6(&builder_);
            Label label7(&builder_);
            auto yPositiveCheck = builder_.DoubleGreaterThanOrEqual(y, builder_.Double(0.));
            builder_.Branch(yPositiveCheck, &label6, &label7);
            builder_.Bind(&label6);
            {
                result = builder_.DoubleToTaggedDoublePtr(builder_.Double(+0.));
                builder_.Jump(&exit);
            }
            builder_.Bind(&label7);
            {
                result = builder_.DoubleToTaggedDoublePtr(builder_.Double(-0.));
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

}

