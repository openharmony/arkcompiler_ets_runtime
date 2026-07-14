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
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(glue, left, right, callback);
    IR_IF (TaggedIsHole(*result)) {
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(Eq), { left, right });
    } IR_ELSE {
        IR_IF (TaggedIsTrue(*result)) {
            callback.ProfileBranch(true);
        } IR_ELSE {
            callback.ProfileBranch(false);
        }
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::NotEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(glue, left, right, callback);
    IR_IF (TaggedIsHole(*result)) {
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(NotEq), { left, right });
    } IR_ELSE {
        IR_IF (TaggedIsTrue(*result)) {
            result = TaggedFalse();
            callback.ProfileBranch(false);
        } IR_ELSE {
            callback.ProfileBranch(true);
            result = TaggedTrue();
        }
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::StrictEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedTrue());
    IR_IF (FastStrictEqual(glue, left, right, callback)) {
        callback.ProfileBranch(true);
    } IR_ELSE {
        result = TaggedFalse();
        callback.ProfileBranch(false);
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::StrictNotEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedTrue());
    IR_IF (FastStrictEqual(glue, left, right, callback)) {
        result = TaggedFalse();
        callback.ProfileBranch(false);
    } IR_ELSE {
        callback.ProfileBranch(true);
    }
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
    Label leftLessRight(env);
    Label leftNotLessRight(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef bothInts = LogicAndBuilder(env).And(TaggedIsInt(left)).And(TaggedIsInt(right)).Done();
    IR_IF (bothInts) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
        GateRef intLeft = TaggedGetInt(left);
        GateRef intRight = TaggedGetInt(right);
        BRANCH(Int32LessThan(intLeft, intRight), &leftLessRight, &leftNotLessRight);
    } IR_ELSE {
        GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
        IR_IF (bothNums) {
            DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
            DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
            DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
            IR_IF (TaggedIsInt(left)) {
                doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
            } IR_ELSE {
                curType = TaggedInt(PGOSampleType::DoubleType());
                doubleLeft = GetDoubleOfTDouble(left);
            }
            IR_IF (TaggedIsInt(right)) {
                GateRef type = TaggedInt(PGOSampleType::IntType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
            } IR_ELSE {
                GateRef type = TaggedInt(PGOSampleType::DoubleType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = GetDoubleOfTDouble(right);
            }
            BRANCH(DoubleLessThan(*doubleLeft, *doubleRight), &leftLessRight, &leftNotLessRight);
        } IR_ELSE {
            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
            result = CallRuntime(glue, RTSTUB_ID(Less), { left, right });
            Jump(&exit);
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
    Label leftLessEqRight(env);
    Label leftNotLessEqRight(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef bothInts = LogicAndBuilder(env).And(TaggedIsInt(left)).And(TaggedIsInt(right)).Done();
    IR_IF (bothInts) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
        GateRef intLeft = TaggedGetInt(left);
        GateRef intRight = TaggedGetInt(right);
        BRANCH(Int32LessThanOrEqual(intLeft, intRight), &leftLessEqRight, &leftNotLessEqRight);
    } IR_ELSE {
        GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
        IR_IF (bothNums) {
            DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
            DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
            DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
            IR_IF (TaggedIsInt(left)) {
                doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
            } IR_ELSE {
                curType = TaggedInt(PGOSampleType::DoubleType());
                doubleLeft = GetDoubleOfTDouble(left);
            }
            IR_IF (TaggedIsInt(right)) {
                GateRef type = TaggedInt(PGOSampleType::IntType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
            } IR_ELSE {
                GateRef type = TaggedInt(PGOSampleType::DoubleType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = GetDoubleOfTDouble(right);
            }
            BRANCH(DoubleLessThanOrEqual(*doubleLeft, *doubleRight), &leftLessEqRight, &leftNotLessEqRight);
        } IR_ELSE {
            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
            result = CallRuntime(glue, RTSTUB_ID(LessEq), { left, right });
            Jump(&exit);
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
    Label leftGreaterRight(env);
    Label leftNotGreaterRight(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef bothInts = LogicAndBuilder(env).And(TaggedIsInt(left)).And(TaggedIsInt(right)).Done();
    IR_IF (bothInts) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
        GateRef intLeft = TaggedGetInt(left);
        GateRef intRight = TaggedGetInt(right);
        BRANCH(Int32GreaterThan(intLeft, intRight), &leftGreaterRight, &leftNotGreaterRight);
    } IR_ELSE {
        GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
        IR_IF (bothNums) {
            DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
            DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
            DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
            IR_IF (TaggedIsInt(left)) {
                doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
            } IR_ELSE {
                curType = TaggedInt(PGOSampleType::DoubleType());
                doubleLeft = GetDoubleOfTDouble(left);
            }
            IR_IF (TaggedIsInt(right)) {
                GateRef type = TaggedInt(PGOSampleType::IntType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
            } IR_ELSE {
                GateRef type = TaggedInt(PGOSampleType::DoubleType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = GetDoubleOfTDouble(right);
            }
            BRANCH(DoubleGreaterThan(*doubleLeft, *doubleRight), &leftGreaterRight, &leftNotGreaterRight);
        } IR_ELSE {
            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
            result = CallRuntime(glue, RTSTUB_ID(Greater), { left, right });
            Jump(&exit);
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
    Label leftGreaterEqRight(env);
    Label leftNotGreaterEQRight(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef bothInts = LogicAndBuilder(env).And(TaggedIsInt(left)).And(TaggedIsInt(right)).Done();
    IR_IF (bothInts) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
        GateRef intLeft = TaggedGetInt(left);
        GateRef intRight = TaggedGetInt(right);
        BRANCH(Int32GreaterThanOrEqual(intLeft, intRight), &leftGreaterEqRight, &leftNotGreaterEQRight);
    } IR_ELSE {
        GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
        IR_IF (bothNums) {
            DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
            DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
            DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
            IR_IF (TaggedIsInt(left)) {
                doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
            } IR_ELSE {
                curType = TaggedInt(PGOSampleType::DoubleType());
                doubleLeft = GetDoubleOfTDouble(left);
            }
            IR_IF (TaggedIsInt(right)) {
                GateRef type = TaggedInt(PGOSampleType::IntType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
            } IR_ELSE {
                GateRef type = TaggedInt(PGOSampleType::DoubleType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = GetDoubleOfTDouble(right);
            }
            BRANCH(DoubleGreaterThanOrEqual(*doubleLeft, *doubleRight), &leftGreaterEqRight, &leftNotGreaterEQRight);
        } IR_ELSE {
            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
            result = CallRuntime(glue, RTSTUB_ID(GreaterEq), { left, right });
            Jump(&exit);
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
    DEFVARIABLE(result, VariableType::JS_ANY(), FastAdd(glue, left, right, callback));
    IR_IF (TaggedIsHole(*result)) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Add2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Sub(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastSub(glue, left, right, callback));
    IR_IF (TaggedIsHole(*result)) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Sub2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Mul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastMul(glue, left, right, callback));
    IR_IF (TaggedIsHole(*result)) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Mul2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Div(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastDiv(left, right, callback));
    IR_IF (TaggedIsHole(*result)) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Div2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Mod(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), FastMod(glue, left, right, callback));
    IR_IF (TaggedIsHole(*result)) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Mod2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Shl(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
    IR_IF (bothNums) {
        IR_IF (TaggedIsInt(left)) {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        } IR_ELSE {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        }
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef val = Int32LSL(*opNumber0, shift);
        result = IntToTaggedPtr(val);
    } IR_ELSE {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Shl2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Shr(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));
    DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::None()));

    GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
    IR_IF (bothNums) {
        IR_IF (TaggedIsInt(left)) {
            IR_IF (TaggedIsInt(right)) {
                curType = TaggedInt(PGOSampleType::IntType());
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                curType = TaggedInt(PGOSampleType::NumberType());
                GateRef rightDouble = GetDoubleOfTDouble(right);
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        } IR_ELSE {
            IR_IF (TaggedIsInt(right)) {
                curType = TaggedInt(PGOSampleType::NumberType());
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                curType = TaggedInt(PGOSampleType::DoubleType());
                GateRef rightDouble = GetDoubleOfTDouble(right);
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        }
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef val = Int32LSR(*opNumber0, shift);
        IR_IF (Int32UnsignedGreaterThan(val, Int32(INT32_MAX))) {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntOverFlowType()));
            result = DoubleToTaggedDoublePtr(ChangeUInt32ToFloat64(val));
        } IR_ELSE {
            callback.ProfileOpType(*curType);
            result = IntToTaggedPtr(val);
        }
    } IR_ELSE {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Shr2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Ashr(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
    IR_IF (bothNums) {
        IR_IF (TaggedIsInt(left)) {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        } IR_ELSE {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        }
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef val = Int32ASR(*opNumber0, shift);
        result = IntToTaggedPtr(val);
    } IR_ELSE {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Ashr2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::And(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
    IR_IF (bothNums) {
        IR_IF (TaggedIsInt(left)) {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                opNumber0 = GetInt32OfTInt(left);
                GateRef rightDouble = GetDoubleOfTDouble(right);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        } IR_ELSE {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        }
        GateRef val = Int32And(*opNumber0, *opNumber1);
        result = IntToTaggedPtr(val);
    } IR_ELSE {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(And2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Or(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
    IR_IF (bothNums) {
        IR_IF (TaggedIsInt(left)) {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        } IR_ELSE {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        }
        GateRef val = Int32Or(*opNumber0, *opNumber1);
        result = IntToTaggedPtr(val);
    } IR_ELSE {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Or2), { left, right });
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Xor(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    GateRef bothNums = LogicAndBuilder(env).And(TaggedIsNumber(left)).And(TaggedIsNumber(right)).Done();
    IR_IF (bothNums) {
        IR_IF (TaggedIsInt(left)) {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                opNumber0 = GetInt32OfTInt(left);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        } IR_ELSE {
            IR_IF (TaggedIsInt(right)) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = GetInt32OfTInt(right);
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
                GateRef rightDouble = GetDoubleOfTDouble(right);
                GateRef leftDouble = GetDoubleOfTDouble(left);
                opNumber0 = DoubleToInt(glue, leftDouble);
                opNumber1 = DoubleToInt(glue, rightDouble);
            }
        }
        GateRef val = Int32Xor(*opNumber0, *opNumber1);
        result = IntToTaggedPtr(val);
    } IR_ELSE {
        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
        result = CallRuntime(glue, RTSTUB_ID(Xor2), { left, right });
    }
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
        BRANCH(TaggedIsDouble(value), &valueIsDouble, &slowPath);
        Bind(&valueIsDouble);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = DoubleToTaggedDoublePtr(DoubleAdd(valueDouble, Double(1.0)));
            Jump(&exit);
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
        BRANCH(TaggedIsDouble(value), &valueIsDouble, &slowPath);
        Bind(&valueIsDouble);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = DoubleToTaggedDoublePtr(DoubleSub(valueDouble, Double(1.0)));
            Jump(&exit);
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

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    IR_IF (TaggedIsInt(value)) {
        GateRef valueInt = GetInt32OfTInt(value);
        IR_IF (Int32Equal(valueInt, Int32(0))) {
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntOverFlowType()));
            result = DoubleToTaggedDoublePtr(Double(-0.0));
        } IR_ELSE {
            IR_IF (Int32Equal(valueInt, Int32(INT32_MIN))) {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntOverFlowType()));
                result = DoubleToTaggedDoublePtr(Double(-static_cast<double>(INT32_MIN)));
            } IR_ELSE {
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                result = IntToTaggedPtr(Int32Sub(Int32(0), valueInt));
            }
        }
    } IR_ELSE {
        IR_IF (TaggedIsDouble(value)) {
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = DoubleToTaggedDoublePtr(DoubleMul(Double(-1), valueDouble));
        } IR_ELSE {
            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
            result = CallRuntime(glue, RTSTUB_ID(Neg), { value });
        }
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::Not(GateRef glue, GateRef value, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    IR_IF (TaggedIsInt(value)) {
        callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
        GateRef valueInt = GetInt32OfTInt(value);
        result = IntToTaggedPtr(Int32Not(valueInt));
    } IR_ELSE {
        IR_IF (TaggedIsDouble(value)) {
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            GateRef valueDouble = GetDoubleOfTDouble(value);
            result = IntToTaggedPtr(Int32Not(DoubleToInt(glue, valueDouble)));
        } IR_ELSE {
            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
            result = CallRuntime(glue, RTSTUB_ID(Not), { value });
        }
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
