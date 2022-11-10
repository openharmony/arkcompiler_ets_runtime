/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins_lowering.h"

namespace panda::ecmascript::kungfu {
void BuiltinLowering::LowerTypedCallBuitin(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto bitfield = acc_.GetBitField(gate);
    auto idGate = acc_.GetValueIn(gate, bitfield - 1);
    auto id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetBitField(idGate));
    switch (id) {
        case BUILTINS_STUB_ID(SQRT):
            LowerTypedSqrt(gate);
            break;
        case BUILTINS_STUB_ID(COS):
        case BUILTINS_STUB_ID(SIN):
        case BUILTINS_STUB_ID(ACOS):
        case BUILTINS_STUB_ID(ATAN):
            LowerTypedTrigonometric(gate, id);
            break;
        default:
            break;
    }
}

void BuiltinLowering::LowerTypedTrigonometric(GateRef gate, BuiltinsStubCSigns::ID id)
{
    auto ret = TypeTrigonometric(gate, id);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

GateRef BuiltinLowering::TypeTrigonometric(GateRef gate, BuiltinsStubCSigns::ID id)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    Label numberBranch(&builder_);
    Label notNumberBranch(&builder_);
    Label exit(&builder_);

    GateRef a0 = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());

    builder_.Branch(builder_.TaggedIsNumber(a0), &numberBranch, &notNumberBranch);
    builder_.Bind(&numberBranch);
    {
        Label IsInt(&builder_);
        Label NotInt(&builder_);
        Label calc(&builder_);
        DEFVAlUE(value, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
        builder_.Branch(builder_.TaggedIsInt(a0), &IsInt, &NotInt);
        builder_.Bind(&IsInt);
        {
            value = builder_.ChangeInt32ToFloat64(builder_.GetInt32OfTInt(a0));
            builder_.Jump(&calc);
        }
        builder_.Bind(&NotInt);
        {
            value = builder_.GetDoubleOfTDouble(a0);
            builder_.Jump(&calc);
        }
        builder_.Bind(&calc);
        {
            Label IsNan(&builder_);
            Label NotNan(&builder_);
            GateRef condition = builder_.DoubleIsNAN(*value);
            builder_.Branch(condition, &IsNan, &NotNan);
            builder_.Bind(&NotNan);
            {
                ArgumentAccessor argAcc_(circuit_);
                auto glue = argAcc_.GetCommonArgGate(CommonArgIdx::GLUE);
                int index = RTSTUB_ID(FloatCos);
                switch (id) {
                    case BUILTINS_STUB_ID(ACOS):
                        index = RTSTUB_ID(FloatACos);
                        break;
                    case BUILTINS_STUB_ID(ATAN):
                        index = RTSTUB_ID(FloatATan);
                        break;
                    case BUILTINS_STUB_ID(COS):
                        index = RTSTUB_ID(FloatCos);
                        break;
                    case BUILTINS_STUB_ID(SIN):
                        index = RTSTUB_ID(FloatSin);
                        break;
                    default:
                        UNREACHABLE();
                }
                result = builder_.CallNGCRuntime(glue, index, Gate::InvalidGateRef, {*value});
                builder_.Jump(&exit);
            }
            builder_.Bind(&IsNan);
            {
                result = builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&notNumberBranch);
    {
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinLowering::LowerTypedSqrt(GateRef gate)
{
    auto ret = TypedSqrt(gate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

GateRef BuiltinLowering::TypedSqrt(GateRef gate)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    Label numberBranch(&builder_);
    Label notNumberBranch(&builder_);
    Label exit(&builder_);
    GateRef a0 = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());

    builder_.Branch(builder_.TaggedIsNumber(a0), &numberBranch, &notNumberBranch);
    builder_.Bind(&numberBranch);
    {
        Label IsInt(&builder_);
        Label NotInt(&builder_);
        Label calc(&builder_);
        DEFVAlUE(value, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
        builder_.Branch(builder_.TaggedIsInt(a0), &IsInt, &NotInt);
        builder_.Bind(&IsInt);
        {
            value = builder_.ChangeInt32ToFloat64(builder_.GetInt32OfTInt(a0));
            builder_.Jump(&calc);
        }
        builder_.Bind(&NotInt);
        {
            value = builder_.GetDoubleOfTDouble(a0);
            builder_.Jump(&calc);
        }
        builder_.Bind(&calc);
        {
            Label signbit(&builder_);
            Label notSignbit(&builder_);
            GateRef negativeInfinity = builder_.Double(-base::POSITIVE_INFINITY);
            GateRef isNegativeInfinity = builder_.Equal(*value, negativeInfinity);
            isNegativeInfinity = builder_.Equal(builder_.SExtInt1ToInt32(isNegativeInfinity), builder_.Int32(1));
            GateRef negativeNan = builder_.Double(-base::NAN_VALUE);
            GateRef isNegativeNan = builder_.Equal(*value, negativeNan);
            isNegativeNan = builder_.Equal(builder_.SExtInt1ToInt32(isNegativeNan), builder_.Int32(1));
            GateRef negativeZero = builder_.Double(-0.0);
            GateRef isNegativeZero = builder_.Equal(*value, negativeZero);
            // If value is negative, include -NaN and -Infinity but not -0.0, the result is NaN
            GateRef negNanOrInfinityNotZero = builder_.BoolOr(isNegativeNan, isNegativeInfinity);
            negNanOrInfinityNotZero = builder_.BoolAnd(negNanOrInfinityNotZero, builder_.BoolNot(isNegativeZero));
            builder_.Branch(negNanOrInfinityNotZero, &signbit, &notSignbit);
            builder_.Bind(&signbit);
            {
                result = builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                builder_.Jump(&exit);
            }
            builder_.Bind(&notSignbit);
            {
                Label naN(&builder_);
                Label notNan(&builder_);
                GateRef condition = builder_.DoubleIsNAN(*value);
                builder_.Branch(condition, &naN, &notNan);
                builder_.Bind(&notNan);
                {
                    ArgumentAccessor argAcc_(circuit_);
                    auto glue = argAcc_.GetCommonArgGate(CommonArgIdx::GLUE);
                    result = builder_.CallNGCRuntime(
                        glue, RTSTUB_ID(FloatSqrt), Gate::InvalidGateRef, {*value});
                    builder_.Jump(&exit);
                }
                // If value is NaN, the result is NaN
                builder_.Bind(&naN);
                {
                    result =  builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&notNumberBranch);
    {
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinLowering::LowerCallTargetCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef function = acc_.GetValueIn(gate, 0); // 0: function
    GateRef id = acc_.GetValueIn(gate, 1); // 1: buitin id
    Label isHeapObject(&builder_);
    Label notHeapObject(&builder_);
    Label funcIsCallable(&builder_);
    Label exit(&builder_);
    GateRef isObject = builder_.TaggedIsHeapObject(function);
    DEFVAlUE(result, (&builder_), VariableType::BOOL(), builder_.False());
    builder_.Branch(isObject, &isHeapObject, &notHeapObject);
    builder_.Bind(&notHeapObject);
    {
        builder_.Jump(&exit);
    }

    builder_.Bind(&isHeapObject);
    {
        GateRef callable = builder_.IsCallable(function);
        builder_.Branch(callable, &funcIsCallable, &exit);
        builder_.Bind(&funcIsCallable);
        {
            GateRef method = builder_.Load(VariableType::JS_ANY(), function,
                builder_.IntPtr(JSFunctionBase::METHOD_OFFSET));
            GateRef builtinId = builder_.GetCallBuiltinId(method);
            result = builder_.Int64Equal(builtinId, id);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

BuiltinsStubCSigns::ID BuiltinLowering::GetBuiltinId(std::string idStr)
{
    const std::map<std::string, BuiltinsStubCSigns::ID> str2BuiltinId = {
        {"sqrt", BUILTINS_STUB_ID(SQRT)},
        {"cos", BUILTINS_STUB_ID(COS)},
        {"sin", BUILTINS_STUB_ID(SIN)},
        {"acos", BUILTINS_STUB_ID(ACOS)},
        {"atan", BUILTINS_STUB_ID(ATAN)},
    };
    if (str2BuiltinId.count(idStr) > 0) {
        return str2BuiltinId.at(idStr);
    }
    return BUILTINS_STUB_ID(NONE);
}

GateRef BuiltinLowering::CheckPara(GateRef gate, BuiltinsStubCSigns::ID id)
{
    GateRef a0 = acc_.GetValueIn(gate, 1);
    GateRef paracheck = builder_.TaggedIsNumber(a0);
    switch (id) {
        case BuiltinsStubCSigns::ID::SQRT:
        case BuiltinsStubCSigns::ID::COS:
        case BuiltinsStubCSigns::ID::SIN:
        case BuiltinsStubCSigns::ID::ACOS:
        case BuiltinsStubCSigns::ID::ATAN: {
            paracheck = builder_.TaggedIsNumber(a0);
            break;
        }
        default: {
            UNREACHABLE();
        }
    }
    return paracheck;
}
}  // namespace panda::ecmascript::kungfu