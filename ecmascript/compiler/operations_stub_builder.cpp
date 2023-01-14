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

#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
GateRef OperationsStubBuilder::Equal(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHole(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(left, right);
    Branch(TaggedIsHole(*result), &isHole, &exit);
    Bind(&isHole);
    {
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(Eq), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::NotEqual(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(left, right);
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(NotEq), { left, right });
        Jump(&exit);
    }
    Bind(&notHole);
    {
        Label resultIsTrue(env);
        Label resultNotTrue(env);
        Branch(TaggedIsTrue(*result), &resultIsTrue, &resultNotTrue);
        Bind(&resultIsTrue);
        {
            result = TaggedFalse();
            Jump(&exit);
        }
        Bind(&resultNotTrue);
        {
            result = TaggedTrue();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Less(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label leftIsInt(env);
    Label leftOrRightNotInt(env);
    Label leftLessRight(env);
    Label leftNotLessRight(env);
    Label slowPath(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            Branch(Int32LessThan(intLeft, intRight), &leftLessRight, &leftNotLessRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    Branch(DoubleLessThan(*doubleLeft, *doubleRight), &leftLessRight, &leftNotLessRight);
                }
            }
        }
    }
    Bind(&leftLessRight);
    {
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&leftNotLessRight);
    {
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Less), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::LessEq(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label leftIsInt(env);
    Label leftOrRightNotInt(env);
    Label leftLessEqRight(env);
    Label leftNotLessEqRight(env);
    Label slowPath(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            Branch(Int32LessThanOrEqual(intLeft, intRight), &leftLessEqRight, &leftNotLessEqRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    Branch(DoubleLessThanOrEqual(*doubleLeft, *doubleRight), &leftLessEqRight, &leftNotLessEqRight);
                }
            }
        }
    }
    Bind(&leftLessEqRight);
    {
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&leftNotLessEqRight);
    {
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(LessEq), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Greater(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label leftIsInt(env);
    Label leftOrRightNotInt(env);
    Label leftGreaterRight(env);
    Label leftNotGreaterRight(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            Branch(Int32GreaterThan(intLeft, intRight), &leftGreaterRight, &leftNotGreaterRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    Branch(DoubleGreaterThan(*doubleLeft, *doubleRight), &leftGreaterRight, &leftNotGreaterRight);
                }
            }
        }
    }
    Bind(&leftGreaterRight);
    {
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&leftNotGreaterRight);
    {
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Greater), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::GreaterEq(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label leftIsInt(env);
    Label leftOrRightNotInt(env);
    Label leftGreaterEqRight(env);
    Label leftNotGreaterEQRight(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            Branch(Int32GreaterThanOrEqual(intLeft, intRight), &leftGreaterEqRight, &leftNotGreaterEQRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    Branch(DoubleGreaterThanOrEqual(*doubleLeft, *doubleRight),
                        &leftGreaterEqRight, &leftNotGreaterEQRight);
                }
            }
        }
    }
    Bind(&leftGreaterEqRight);
    {
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&leftNotGreaterEQRight);
    {
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(GreaterEq), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Add(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastAdd(left, right));
    Branch(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Add2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Sub(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastSub(left, right));
    Branch(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Sub2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Mul(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastMul(left, right));
    Branch(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Mul2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Div(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastDiv(left, right));
    Branch(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Div2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Mod(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastMod(glue, left, right));
    Branch(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Mod2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Shl(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    Label calculate(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        result = CallRuntime(glue, RTSTUB_ID(Shl2), { left, right });
        Jump(&exit);
    }
    Bind(&calculate);
    {
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef val = Int32LSL(*opNumber0, shift);
        result = IntToTaggedPtr(val);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Shr(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    Label doShr(env);
    Label overflow(env);
    Label notOverflow(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&doShr);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&doShr);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&doShr);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&doShr);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        result = CallRuntime(glue, RTSTUB_ID(Shr2), { left, right });
        Jump(&exit);
    }
    Bind(&doShr);
    {
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef val = Int32LSR(*opNumber0, shift);
        auto condition = Int32UnsignedGreaterThan(val, Int32(INT32_MAX));
        Branch(condition, &overflow, &notOverflow);
        Bind(&overflow);
        {
            result = DoubleToTaggedDoublePtr(ChangeUInt32ToFloat64(val));
            Jump(&exit);
        }
        Bind(&notOverflow);
        {
            result = IntToTaggedPtr(val);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Ashr(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    Label calculate(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        result = CallRuntime(glue, RTSTUB_ID(Ashr2), { left, right });
        Jump(&exit);
    }
    Bind(&calculate);
    {
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef val = Int32ASR(*opNumber0, shift);
        result = IntToTaggedPtr(val);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::And(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    Label calculate(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    opNumber0 = GetInt32OfTInt(left);
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        result = CallRuntime(glue, RTSTUB_ID(And2), { left, right });
        Jump(&exit);
    }
    Bind(&calculate);
    {
        GateRef val = Int32And(*opNumber0, *opNumber1);
        result = IntToTaggedPtr(val);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Or(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    Label calculate(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        result = CallRuntime(glue, RTSTUB_ID(Or2), { left, right });
        Jump(&exit);
    }
    Bind(&calculate);
    {
        GateRef val = Int32Or(*opNumber0, *opNumber1);
        result = IntToTaggedPtr(val);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Xor(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    Label calculate(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = GetDoubleOfTDouble(right);
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&calculate);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        result = CallRuntime(glue, RTSTUB_ID(Xor2), { left, right });
        Jump(&exit);
    }
    Bind(&calculate);
    {
        GateRef val = Int32Xor(*opNumber0, *opNumber1);
        result = IntToTaggedPtr(val);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Inc(GateRef glue, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label slowPath(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = GetInt32OfTInt(value);
        Label valueNoOverflow(env);
        Branch(Int32Equal(valueInt, Int32(INT32_MAX)), &valueNotInt, &valueNoOverflow);
        Bind(&valueNoOverflow);
        {
            result = IntToTaggedPtr(Int32Add(valueInt, Int32(1)));
            Jump(&exit);
        }
    }
    Bind(&valueNotInt);
    {
        Label valueIsDouble(env);
        Branch(TaggedIsDouble(value), &valueIsDouble, &slowPath);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = DoubleToTaggedDoublePtr(DoubleAdd(valueDouble, Double(1.0)));
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Inc), { value });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Dec(GateRef glue, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label slowPath(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = GetInt32OfTInt(value);
        Label valueNoOverflow(env);
        Branch(Int32Equal(valueInt, Int32(INT32_MIN)), &valueNotInt, &valueNoOverflow);
        Bind(&valueNoOverflow);
        {
            result = IntToTaggedPtr(Int32Sub(valueInt, Int32(1)));
            Jump(&exit);
        }
    }
    Bind(&valueNotInt);
    {
        Label valueIsDouble(env);
        Branch(TaggedIsDouble(value), &valueIsDouble, &slowPath);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = DoubleToTaggedDoublePtr(DoubleSub(valueDouble, Double(1.0)));
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(Dec), { value });
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Neg(GateRef glue, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label valueIsInt(env);
    Label valueNotInt(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = GetInt32OfTInt(value);
        Label valueIsZero(env);
        Label valueNotZero(env);
        Branch(Int32Equal(valueInt, Int32(0)), &valueIsZero, &valueNotZero);
        Bind(&valueIsZero);
        {
            result = DoubleToTaggedDoublePtr(Double(-0.0));
            Jump(&exit);
        }
        Bind(&valueNotZero);
        {
            Label valueIsInt32Min(env);
            Label valueNotInt32Min(env);
            Branch(Int32Equal(valueInt, Int32(INT32_MIN)), &valueIsInt32Min, &valueNotInt32Min);
            Bind(&valueIsInt32Min);
            {
                result = DoubleToTaggedDoublePtr(Double(-static_cast<double>(INT32_MIN)));
                Jump(&exit);
            }
            Bind(&valueNotInt32Min);
            {
                result = IntToTaggedPtr(Int32Sub(Int32(0), valueInt));
                Jump(&exit);
            }
        }
    }
    Bind(&valueNotInt);
    {
        Label valueIsDouble(env);
        Label valueNotDouble(env);
        Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = DoubleToTaggedDoublePtr(DoubleSub(Double(0), valueDouble));
            Jump(&exit);
        }
        Bind(&valueNotDouble);
        {
            // slow path
            result = CallRuntime(glue, RTSTUB_ID(Neg), { value });
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Not(GateRef glue, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label numberIsInt(env);
    Label numberNotInt(env);
    Branch(TaggedIsInt(value), &numberIsInt, &numberNotInt);
    Bind(&numberIsInt);
    {
        GateRef valueInt = GetInt32OfTInt(value);
        result = IntToTaggedPtr(Int32Not(valueInt));
        Jump(&exit);
    }
    Bind(&numberNotInt);
    {
        Label numberIsDouble(env);
        Label numberNotDouble(env);
        Branch(TaggedIsDouble(value), &numberIsDouble, &numberNotDouble);
        Bind(&numberIsDouble);
        {
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = IntToTaggedPtr(Int32Not(DoubleToInt(glue, valueDouble)));
            Jump(&exit);
        }
        Bind(&numberNotDouble);
        {
            // slow path
            result = CallRuntime(glue, RTSTUB_ID(Not), { value });
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
