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
        case OpCode::MATH_SIN:
            LowerMathTrigonometric(gate, RTSTUB_ID(FloatSin));
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

void TypedNativeInlineLowering::LowerMathTrigonometric(GateRef gate, RuntimeStubCSigns::ID stub_id)
{
    Environment env(gate, circuit_, &builder_);
    builder_.SetEnvironment(&env);

    Label exit(&builder_);
    Label NotNan(&builder_);

    GateRef param = acc_.GetValueIn(gate, 0);
    auto nan_value = builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), nan_value);

    GateRef value = builder_.GetDoubleOfTNumber(param);
    builder_.Branch(builder_.DoubleIsNAN(value), &exit, &NotNan);
    builder_.Bind(&NotNan);
    {
        GateRef glue = acc_.GetGlueFromArgList();
        result = builder_.CallNGCRuntime(glue, stub_id, Gate::InvalidGateRef, {value}, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

}

