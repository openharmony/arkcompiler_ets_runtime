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
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
GateRef OperationsStubBuilder::Equal(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHole(env);
    Label notHole(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(glue, left, right, callback);
    BRANCH(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(Eq), { left, right });
        Jump(&exit);
    }
    Bind(&notHole);
    {
        Label resultIsTrue(env);
        Label resultNotTrue(env);
        BRANCH(TaggedIsTrue(*result), &resultIsTrue, &resultNotTrue);
        Bind(&resultIsTrue);
        {
            callback.ProfileBranch(true);
            Jump(&exit);
        }
        Bind(&resultNotTrue);
        {
            callback.ProfileBranch(false);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::NotEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(glue, left, right, callback);
    Label isHole(env);
    Label notHole(env);
    BRANCH(TaggedIsHole(*result), &isHole, &notHole);
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
        BRANCH(TaggedIsTrue(*result), &resultIsTrue, &resultNotTrue);
        Bind(&resultIsTrue);
        {
            result = TaggedFalse();
            callback.ProfileBranch(false);
            Jump(&exit);
        }
        Bind(&resultNotTrue);
        {
            callback.ProfileBranch(true);
            result = TaggedTrue();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::StrictEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label strictEqual(env);
    Label notStrictEqual(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedTrue());
    BRANCH(FastStrictEqual(glue, left, right, callback), &strictEqual, &notStrictEqual);
    Bind(&strictEqual);
    {
        callback.ProfileBranch(true);
        Jump(&exit);
    }
    Bind(&notStrictEqual);
    {
        result = TaggedFalse();
        callback.ProfileBranch(false);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::StrictNotEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label strictEqual(env);
    Label notStrictEqual(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedTrue());
    BRANCH(FastStrictEqual(glue, left, right, callback), &strictEqual, &notStrictEqual);
    Bind(&strictEqual);
    {
        result = TaggedFalse();
        callback.ProfileBranch(false);
        Jump(&exit);
    }
    Bind(&notStrictEqual);
    {
        callback.ProfileBranch(true);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Less(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        BRANCH(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            BRANCH(Int32LessThan(intLeft, intRight), &leftLessRight, &leftNotLessRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        BRANCH(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            BRANCH(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                BRANCH(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    curType = TaggedInt(PGOSampleType::DoubleType());
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    BRANCH(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::IntType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::DoubleType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    BRANCH(DoubleLessThan(*doubleLeft, *doubleRight), &leftLessRight, &leftNotLessRight);
                }
            }
        }
    }
    Bind(&leftLessRight);
    {
        callback.ProfileBranch(true);
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&leftNotLessRight);
    {
        callback.ProfileBranch(false);
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Less), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::LessEq(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        BRANCH(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            BRANCH(Int32LessThanOrEqual(intLeft, intRight), &leftLessEqRight, &leftNotLessEqRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        BRANCH(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            BRANCH(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                BRANCH(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    curType = TaggedInt(PGOSampleType::DoubleType());
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    BRANCH(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::IntType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::DoubleType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    BRANCH(DoubleLessThanOrEqual(*doubleLeft, *doubleRight), &leftLessEqRight, &leftNotLessEqRight);
                }
            }
        }
    }
    Bind(&leftLessEqRight);
    {
        callback.ProfileBranch(true);
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&leftNotLessEqRight);
    {
        callback.ProfileBranch(false);
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(LessEq), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Greater(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        BRANCH(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            BRANCH(Int32GreaterThan(intLeft, intRight), &leftGreaterRight, &leftNotGreaterRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        BRANCH(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            BRANCH(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                BRANCH(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    curType = TaggedInt(PGOSampleType::DoubleType());
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    BRANCH(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::IntType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::DoubleType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    BRANCH(DoubleGreaterThan(*doubleLeft, *doubleRight), &leftGreaterRight, &leftNotGreaterRight);
                }
            }
        }
    }
    Bind(&leftGreaterRight);
    {
        callback.ProfileBranch(true);
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&leftNotGreaterRight);
    {
        callback.ProfileBranch(false);
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Greater), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::GreaterEq(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        BRANCH(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            BRANCH(Int32GreaterThanOrEqual(intLeft, intRight), &leftGreaterEqRight, &leftNotGreaterEQRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        BRANCH(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            BRANCH(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                BRANCH(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    curType = TaggedInt(PGOSampleType::DoubleType());
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    BRANCH(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::IntType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::DoubleType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    BRANCH(DoubleGreaterThanOrEqual(*doubleLeft, *doubleRight),
                        &leftGreaterEqRight, &leftNotGreaterEQRight);
                }
            }
        }
    }
    Bind(&leftGreaterEqRight);
    {
        callback.ProfileBranch(true);
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&leftNotGreaterEQRight);
    {
        callback.ProfileBranch(false);
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(GreaterEq), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Add(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastAdd(glue, left, right, callback));
    BRANCH(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Add2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Sub(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastSub(glue, left, right, callback));
    BRANCH(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Sub2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Mul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastMul(glue, left, right, callback));
    BRANCH(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Mul2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Div(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastDiv(left, right, callback));
    BRANCH(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Div2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Mod(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowPath(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastMod(glue, left, right, callback));
    BRANCH(TaggedIsHole(*result), &slowPath, &exit);
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Mod2), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Shl(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
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
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
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
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
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

GateRef OperationsStubBuilder::Shr(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));
    DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::None()));

    Label doShr(env);
    Label overflow(env);
    Label notOverflow(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    curType = TaggedInt(PGOSampleType::IntType());
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&doShr);
                }
                Bind(&rightIsDouble);
                {
                    curType = TaggedInt(PGOSampleType::NumberType());
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
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    curType = TaggedInt(PGOSampleType::NumberType());
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&doShr);
                }
                Bind(&rightIsDouble);
                {
                    curType = TaggedInt(PGOSampleType::DoubleType());
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
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Shr2), { left, right });
        Jump(&exit);
    }
    Bind(&doShr);
    {
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef val = Int32LSR(*opNumber0, shift);
        auto condition = Int32UnsignedGreaterThan(val, Int32(INT32_MAX));
        BRANCH(condition, &overflow, &notOverflow);
        Bind(&overflow);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntOverFlowType()));
            result = DoubleToTaggedDoublePtr(ChangeUInt32ToFloat64(val));
            Jump(&exit);
        }
        Bind(&notOverflow);
        {
            callback.ProfileOpType(*curType);
            result = IntToTaggedPtr(val);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Ashr(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
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
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
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
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
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

GateRef OperationsStubBuilder::And(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
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
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
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
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
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

GateRef OperationsStubBuilder::Or(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
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
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
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
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
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

GateRef OperationsStubBuilder::Xor(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
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
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                    opNumber0 = GetInt32OfTInt(left);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
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
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                    GateRef leftDouble = GetDoubleOfTDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = GetInt32OfTInt(right);
                    Jump(&calculate);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
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
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
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

GateRef OperationsStubBuilder::Inc(GateRef glue, GateRef value, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label slowPath(env);
    BRANCH(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = GetInt32OfTInt(value);
        Label valueNoOverflow(env);
        BRANCH(Int32Equal(valueInt, Int32(INT32_MAX)), &valueNotInt, &valueNoOverflow);
        Bind(&valueNoOverflow);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
            result = IntToTaggedPtr(Int32Add(valueInt, Int32(1)));
            Jump(&exit);
        }
    }
    Bind(&valueNotInt);
    {
        Label valueIsDouble(env);
        Label valueIsNanOrInf(env);
        Label valueIsNotNanOrInf(env);
        BRANCH(TaggedIsDouble(value), &valueIsDouble, &slowPath);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = GetDoubleOfTDouble(value);
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            BRANCH_UNLIKELY(DoubleIsNanOrInf(valueDouble), &valueIsNanOrInf, &valueIsNotNanOrInf);
            Bind(&valueIsNanOrInf);
            {
                result = value;
                Jump(&exit);
            }
            Bind(&valueIsNotNanOrInf);
            {
                result = DoubleToTaggedDoublePtr(DoubleAdd(valueDouble, Double(1.0)));
                Jump(&exit);
            }
        }
    }
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Inc), { value });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Dec(GateRef glue, GateRef value, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label slowPath(env);
    BRANCH(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = GetInt32OfTInt(value);
        Label valueNoOverflow(env);
        BRANCH(Int32Equal(valueInt, Int32(INT32_MIN)), &valueNotInt, &valueNoOverflow);
        Bind(&valueNoOverflow);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
            result = IntToTaggedPtr(Int32Sub(valueInt, Int32(1)));
            Jump(&exit);
        }
    }
    Bind(&valueNotInt);
    {
        Label valueIsDouble(env);
        Label valueIsNanOrInf(env);
        Label valueIsNotNanOrInf(env);
        BRANCH(TaggedIsDouble(value), &valueIsDouble, &slowPath);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = GetDoubleOfTDouble(value);
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            BRANCH_UNLIKELY(DoubleIsNanOrInf(valueDouble), &valueIsNanOrInf, &valueIsNotNanOrInf);
            Bind(&valueIsNanOrInf);
            {
                result = value;
                Jump(&exit);
            }
            Bind(&valueIsNotNanOrInf);
            {
                result = DoubleToTaggedDoublePtr(DoubleSub(valueDouble, Double(1.0)));
                Jump(&exit);
            }
        }
    }
    Bind(&slowPath);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Dec), { value });
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Neg(GateRef glue, GateRef value, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label valueIsInt(env);
    Label valueNotInt(env);
    BRANCH(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = GetInt32OfTInt(value);
        Label valueIsZero(env);
        Label valueNotZero(env);
        BRANCH(Int32Equal(valueInt, Int32(0)), &valueIsZero, &valueNotZero);
        Bind(&valueIsZero);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntOverFlowType()));
            result = DoubleToTaggedDoublePtr(Double(-0.0));
            Jump(&exit);
        }
        Bind(&valueNotZero);
        {
            Label valueIsInt32Min(env);
            Label valueNotInt32Min(env);
            BRANCH(Int32Equal(valueInt, Int32(INT32_MIN)), &valueIsInt32Min, &valueNotInt32Min);
            Bind(&valueIsInt32Min);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntOverFlowType()));
                result = DoubleToTaggedDoublePtr(Double(-static_cast<double>(INT32_MIN)));
                Jump(&exit);
            }
            Bind(&valueNotInt32Min);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                result = IntToTaggedPtr(Int32Sub(Int32(0), valueInt));
                Jump(&exit);
            }
        }
    }
    Bind(&valueNotInt);
    {
        Label valueIsDouble(env);
        Label valueNotDouble(env);
        BRANCH(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
        Bind(&valueIsDouble);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = DoubleToTaggedDoublePtr(DoubleMul(Double(-1), valueDouble));
            Jump(&exit);
        }
        Bind(&valueNotDouble);
        {
            // slow path
            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
            result = CallRuntime(glue, RTSTUB_ID(Neg), { value });
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Not(GateRef glue, GateRef value, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label numberIsInt(env);
    Label numberNotInt(env);
    BRANCH(TaggedIsInt(value), &numberIsInt, &numberNotInt);
    Bind(&numberIsInt);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
        GateRef valueInt = GetInt32OfTInt(value);
        result = IntToTaggedPtr(Int32Not(valueInt));
        Jump(&exit);
    }
    Bind(&numberNotInt);
    {
        Label numberIsDouble(env);
        Label numberNotDouble(env);
        BRANCH(TaggedIsDouble(value), &numberIsDouble, &numberNotDouble);
        Bind(&numberIsDouble);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = IntToTaggedPtr(Int32Not(DoubleToInt(glue, valueDouble)));
            Jump(&exit);
        }
        Bind(&numberNotDouble);
        {
            // slow path
            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
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
