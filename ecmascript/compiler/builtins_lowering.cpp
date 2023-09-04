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
    auto valuesIn = acc_.GetNumValueIn(gate);
    auto idGate = acc_.GetValueIn(gate, valuesIn - 1);
    auto id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(idGate));
    switch (id) {
        case BUILTINS_STUB_ID(ABS):
            LowerTypedAbs(gate);
            break;
        case BUILTINS_STUB_ID(FLOOR):
        case BUILTINS_STUB_ID(COS):
        case BUILTINS_STUB_ID(SIN):
        case BUILTINS_STUB_ID(ACOS):
        case BUILTINS_STUB_ID(ATAN):
            LowerTypedTrigonometric(gate, id);
            break;
        case BUILTINS_STUB_ID(LocaleCompare):
            LowerTypedLocaleCompare(gate);
            break;
        case BUILTINS_STUB_ID(SORT):
            LowerTypedArraySort(gate);
            break;
        case BUILTINS_STUB_ID(STRINGIFY):
            LowerTypedStringify(gate);
            break;
        default:
            break;
    }
}

void BuiltinLowering::LowerTypedTrigonometric(GateRef gate, BuiltinsStubCSigns::ID id)
{
    auto ret = TypedTrigonometric(gate, id);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

GateRef BuiltinLowering::TypedTrigonometric(GateRef gate, BuiltinsStubCSigns::ID id)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    Label numberBranch(&builder_);
    Label notNumberBranch(&builder_);
    Label exit(&builder_);

    GateRef para1 = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());

    builder_.Branch(builder_.TaggedIsNumber(para1), &numberBranch, &notNumberBranch);
    builder_.Bind(&numberBranch);
    {
        GateRef value = builder_.GetDoubleOfTNumber(para1);
        Label IsNan(&builder_);
        Label NotNan(&builder_);
        GateRef condition = builder_.DoubleIsNAN(value);
        builder_.Branch(condition, &IsNan, &NotNan);
        builder_.Bind(&NotNan);
        {
            GateRef glue = acc_.GetGlueFromArgList();
            int index = RTSTUB_ID(FloatCos);
            switch (id) {
                case BUILTINS_STUB_ID(FLOOR):
                    index = RTSTUB_ID(FloatFloor);
                    break;
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
                    LOG_ECMA(FATAL) << "this branch is unreachable";
                    UNREACHABLE();
            }
            result = builder_.CallNGCRuntime(glue, index, Gate::InvalidGateRef, {value}, gate);
            builder_.Jump(&exit);
        }
        builder_.Bind(&IsNan);
        {
            result = builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
            builder_.Jump(&exit);
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
    Environment env(gate, circuit_, &builder_);
    GateRef param = acc_.GetValueIn(gate, 0);
    // 20.2.2.32
    // If value is NAN or negative, include -NaN and -Infinity but not -0.0, the result is NaN
    // Assembly instruction support NAN and negative
    auto ret = builder_.Sqrt(param);
    acc_.SetMachineType(gate, MachineType::F64);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

void BuiltinLowering::LowerTypedAbs(GateRef gate)
{
    auto ret = TypedAbs(gate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

GateRef BuiltinLowering::IntToTaggedIntPtr(GateRef x)
{
    GateRef val = builder_.SExtInt32ToInt64(x);
    return builder_.ToTaggedIntPtr(val);
}

//  Int abs : The internal representation of an integer is inverse code,
//  The absolute value of a negative number can be found by inverting it by adding one.

//  Float abs : A floating-point number is composed of mantissa and exponent.
//  The length of mantissa will affect the precision of the number, and its sign will determine the sign of the number.
//  The absolute value of a floating-point number can be found by setting mantissa sign bit to 0.
GateRef BuiltinLowering::TypedAbs(GateRef gate)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    Label exit(&builder_);
    GateRef para1 = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());

    Label isInt(&builder_);
    Label notInt(&builder_);
    builder_.Branch(builder_.TaggedIsInt(para1), &isInt, &notInt);
    builder_.Bind(&isInt);
    {
        auto value = builder_.GetInt32OfTInt(para1);
        auto temp = builder_.Int32ASR(value, builder_.Int32(JSTaggedValue::INT_SIGN_BIT_OFFSET));
        auto res = builder_.Int32Xor(value, temp);
        result = IntToTaggedIntPtr(builder_.Int32Sub(res, temp));
        builder_.Jump(&exit);
    }
    builder_.Bind(&notInt);
    {
        auto value = builder_.GetDoubleOfTDouble(para1);
        // set the sign bit to 0 by shift left then right.
        auto temp = builder_.Int64LSL(builder_.CastDoubleToInt64(value), builder_.Int64(1));
        auto res = builder_.Int64LSR(temp, builder_.Int64(1));
        result = builder_.DoubleToTaggedDoublePtr(builder_.CastInt64ToFloat64(res));
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinLowering::LowerCallRuntime(GateRef glue, GateRef gate, int index, const std::vector<GateRef> &args,
                                          bool useLabel)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args, gate, name.c_str());
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, builder_.GetDepend(), args, gate, name.c_str());
        return result;
    }
}

void BuiltinLowering::ReplaceHirWithValue(GateRef hirGate, GateRef value, bool noThrow)
{
    if (!noThrow) {
        GateRef state = builder_.GetState();
        // copy depend-wire of hirGate to value
        GateRef depend = builder_.GetDepend();
        // exception value
        GateRef exceptionVal = builder_.ExceptionConstant();
        // compare with trampolines result
        GateRef equal = builder_.Equal(value, exceptionVal);
        auto ifBranch = builder_.Branch(state, equal);

        GateRef ifTrue = builder_.IfTrue(ifBranch);
        GateRef ifFalse = builder_.IfFalse(ifBranch);
        GateRef eDepend = builder_.DependRelay(ifTrue, depend);
        GateRef sDepend = builder_.DependRelay(ifFalse, depend);
        StateDepend success(ifFalse, sDepend);
        StateDepend exception(ifTrue, eDepend);
        acc_.ReplaceHirWithIfBranch(hirGate, success, exception, value);
    } else {
        acc_.ReplaceHirDirectly(hirGate, builder_.GetStateDepend(), value);
    }
}

void BuiltinLowering::LowerTypedLocaleCompare(GateRef gate)
{
    GateRef glue = acc_.GetGlueFromArgList();
    uint32_t index = 0;
    GateRef thisObj = acc_.GetValueIn(gate, index++);
    GateRef a0 = acc_.GetValueIn(gate, index++);
    GateRef a1 = acc_.GetValueIn(gate, index++);
    GateRef a2 = acc_.GetValueIn(gate, index++);

    std::vector<GateRef> args;
    args.reserve(index);
    args.emplace_back(thisObj);
    args.emplace_back(a0);
    args.emplace_back(a1);
    args.emplace_back(a2);
    GateRef result = LowerCallRuntime(glue, gate, RTSTUB_ID(LocaleCompare), args);
    ReplaceHirWithValue(gate, result);
}

void BuiltinLowering::LowerTypedArraySort(GateRef gate)
{
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef result = LowerCallRuntime(glue, gate, RTSTUB_ID(ArraySort), { thisObj });
    ReplaceHirWithValue(gate, result);
}

GateRef BuiltinLowering::LowerCallTargetCheck(Environment *env, GateRef gate)
{
    builder_.SetEnvironment(env);
    GateRef idGate = acc_.GetValueIn(gate, 1);
    BuiltinsStubCSigns::ID id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(idGate));
    GateRef constantFunction = builder_.GetGlobalConstantValue(GET_TYPED_CONSTANT_INDEX(id));

    GateRef function = acc_.GetValueIn(gate, 0); // 0: function
    return builder_.Equal(function, constantFunction);
}

GateRef BuiltinLowering::CheckPara(GateRef gate, GateRef funcCheck)
{
    GateRef idGate = acc_.GetValueIn(gate, 1);
    BuiltinsStubCSigns::ID id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(idGate));
    switch (id) {
        case BuiltinsStubCSigns::ID::COS:
        case BuiltinsStubCSigns::ID::SIN:
        case BuiltinsStubCSigns::ID::ACOS:
        case BuiltinsStubCSigns::ID::ATAN:
        case BuiltinsStubCSigns::ID::ABS:
        case BuiltinsStubCSigns::ID::FLOOR: {
            GateRef para = acc_.GetValueIn(gate, 2);
            GateRef paracheck = builder_.TaggedIsNumber(para);
            return builder_.BoolAnd(paracheck, funcCheck);
        }
        case BuiltinsStubCSigns::ID::SQRT:
            // NumberSpeculativeRetype is checked
            return funcCheck;
        case BuiltinsStubCSigns::ID::LocaleCompare:
        case BuiltinsStubCSigns::ID::SORT:
            // Don't need check para
            return funcCheck;
        default: {
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
}

void BuiltinLowering::LowerTypedStringify(GateRef gate)
{
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef value = acc_.GetValueIn(gate, 0);
    std::vector<GateRef> args;
    args.emplace_back(value);
    GateRef result = LowerCallRuntime(glue, gate, RTSTUB_ID(FastStringify), args);
    ReplaceHirWithValue(gate, result);
}
}  // namespace panda::ecmascript::kungfu
