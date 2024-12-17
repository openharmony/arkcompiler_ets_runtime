/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/common_stubs.h"

#include "ecmascript/compiler/barrier_stub_builder.h"
#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_collection_iterator_stub_builder.h"
#include "ecmascript/compiler/builtins/linked_hashtable_stub_builder.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

void AddStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Add(glue, x, y));
}

void SubStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Sub(glue, x, y));
}

void DefineFieldStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef propKey = TaggedArgument(2);    // 2: 3rd argument
    GateRef acc = TaggedArgument(3);        // 3: 4th argument
    Return(DefineField(glue, receiver, propKey, acc));
}

void DefinefuncStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef jsFunc = TaggedArgument(1);
    GateRef methodId = Int32Argument(2); // 2: 3rd argument
    GateRef length = Int32Argument(3);   // 3: 4th argument
    GateRef lexEnv = TaggedArgument(4);  // 4: 5th argument
    GateRef slotId = Int32Argument(5);   // 5: 6th argument

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label failed(env);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.NewJSFunction(glue, jsFunc, methodId, length, lexEnv, &result, &exit, &failed, slotId);
    Bind(&failed);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    Return(*result);
}

void CallArg0StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLARG0_IMM8);
    Return(callBuilder.CallStubDispatch());
}

void CallArg1StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLARG1_IMM8_V8);
    Return(callBuilder.CallStubDispatch());
}

void CallArg2StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLARGS2_IMM8_V8_V8);
    Return(callBuilder.CallStubDispatch());
}

void CallArg3StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8);
    Return(callBuilder.CallStubDispatch());
}

void CallThis0StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLTHIS0_IMM8_V8);
    Return(callBuilder.CallStubDispatch());
}

void CallThis1StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLTHIS1_IMM8_V8_V8);
    Return(callBuilder.CallStubDispatch());
}

void CallThis2StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8);
    Return(callBuilder.CallStubDispatch());
}

void CallThis3StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8);
    Return(callBuilder.CallStubDispatch());
}

void ConvertCharToInt32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef charCode = Int32Argument(1);
    BuiltinsStringStubBuilder builder(this);
    // char to string
    GateRef result = builder.CreateStringBySingleCharCode(glue, charCode);
    // string to number
    result = CallNGCRuntime(glue, RTSTUB_ID(StringToNumber), {result, Int32(0)}, charCode);
    // get int from number
    result = NumberGetInt(glue, result);
    Return(result);
}

void ConvertCharToDoubleStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef charCode = Int32Argument(1);
    BuiltinsStringStubBuilder builder(this);
    // char to string
    GateRef result = builder.CreateStringBySingleCharCode(glue, charCode);
    // string to number
    result = CallNGCRuntime(glue, RTSTUB_ID(StringToNumber), {result, Int32(0)}, charCode);
    // get double from number
    result = GetDoubleOfTNumber(result);
    Return(result);
}

void MulStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Mul(glue, x, y));
}

void DivStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Div(glue, x, y));
}

void ModStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Mod(glue, x, y));
}

void TypeOfStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    Return(FastTypeOf(glue, obj));
}

void EqualStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Equal(glue, x, y));
}

void NotEqualStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.NotEqual(glue, x, y));
}

void StrictEqualStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.StrictEqual(glue, x, y));
}

void StrictNotEqualStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.StrictNotEqual(glue, x, y));
}

void LessStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Less(glue, x, y));
}

void LessEqStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.LessEq(glue, x, y));
}

void GreaterStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Greater(glue, x, y));
}

void GreaterEqStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.GreaterEq(glue, x, y));
}

void ShlStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Shl(glue, x, y));
}

void ShrStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Shr(glue, x, y));
}

void AshrStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Ashr(glue, x, y));
}

void AndStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.And(glue, x, y));
}

void OrStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Or(glue, x, y));
}

void XorStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Xor(glue, x, y));
}

void IsInStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef prop = TaggedArgument(1); // 1: 2nd argument
    GateRef obj = TaggedArgument(2);  // 2: 3rd argument
    Return(IsIn(glue, prop, obj));
}

void InstanceofStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef object = TaggedArgument(1);
    GateRef target = TaggedArgument(2); // 2: 3rd argument
    GateRef jsFunc = TaggedArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4); // 4 : 5th pars
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(InstanceOf(glue, object, target, profileTypeInfo, slotId, ProfileOperation()));
}

void IncStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Inc(glue, x));
}

void DecStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Dec(glue, x));
}

void NegStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Neg(glue, x));
}

void NotStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    OperationsStubBuilder operationBuilder(this);
    Return(operationBuilder.Not(glue, x));
}

void ToBooleanTrueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    Return(FastToBoolean(x, true));
}

void ToBooleanFalseStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    Return(FastToBoolean(x, false));
}

void NewLexicalEnvStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef parent = TaggedArgument(1);
    GateRef numVars = Int32Argument(2); /* 2 : 3rd parameter is index */

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterNew(env);
    newBuilder.NewLexicalEnv(&result, &afterNew, numVars, parent);
    Bind(&afterNew);
    Return(*result);
}

void CreateObjectHavingMethodStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef env = Int32Argument(2); /* 2 : 3rd parameter is index */

    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    GateRef result = newBuilder.CreateObjectHavingMethod(glue, obj, env);
    Return(result);
}

void CopyRestArgsStubBuilder::GenerateCircuit()
{
    DEFVARIABLE(arrayObj, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(argv, VariableType::NATIVE_POINTER(), PtrArgument(1));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(actualRestNum, VariableType::INT32(), Int32(0));
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef startIdx = Int32Argument(2); /* 2 : 3rd parameter is startIdx */
    GateRef numArgs = Int32Argument(3); /* 3 : 4th parameter is numArgs */
    GateRef argvTaggedArray = TaggedArgument(4); /* 4 : 5th parameter is argvTaggedArray */

    Label numArgsGreater(env);
    Label numArgsNotGreater(env);
    Label afterCreateArrayObj(env);

    GateRef actualArgc = Int32Sub(numArgs, Int32(NUM_MANDATORY_JSFUNC_ARGS));
    // 1. Calculate actual rest num.
    BRANCH(Int32UnsignedGreaterThan(actualArgc, startIdx), &numArgsGreater, &numArgsNotGreater);
    Bind(&numArgsGreater);
    {
        actualRestNum = Int32Sub(actualArgc, startIdx);
        Jump(&numArgsNotGreater);
    }
    Bind(&numArgsNotGreater);
    // 2. Construct RestArguments object.
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    GateRef intialHClass = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                  ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX);
    arrayObj = newBuilder.NewJSArrayWithSize(intialHClass, *actualRestNum);

    GateRef args = GetArgumentsElements(glue, argvTaggedArray, *argv);
    newBuilder.AssignRestArg(&arrayObj, &afterCreateArrayObj, args, startIdx, *actualRestNum);
    Bind(&afterCreateArrayObj);
    Return(*arrayObj);
}

void GetUnmappedArgsStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef numArgs = Int32Argument(2); /* 2 : 3rd parameter is numArgs */
    GateRef argvTaggedArray = TaggedArgument(3); /* 3 : 4th parameter is argvTaggedArray */

    DEFVARIABLE(argumentsList, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(argumentsObj, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(argv, VariableType::NATIVE_POINTER(), PtrArgument(1));
    Label afterArgumentsList(env);
    Label newArgumentsObj(env);
    Label exit(env);

    GateRef actualArgc = Int32Sub(numArgs, Int32(NUM_MANDATORY_JSFUNC_ARGS));
    GateRef startIdx = Int32(0);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);

    Label fillArguments(env);
    Label argumentsException(env);
    GateRef argumentsListObj = newBuilder.NewArgumentsListObj(actualArgc);
    argumentsList.WriteVariable(argumentsListObj);
    Branch(TaggedIsException(*argumentsList), &argumentsException, &fillArguments);
    Bind(&argumentsException);
    argumentsObj.WriteVariable(*argumentsList);
    Jump(&exit);
    Bind(&fillArguments);

    GateRef args = GetArgumentsElements(glue, argvTaggedArray, *argv);
    newBuilder.FillArgumentsList(*argumentsList, args, startIdx, actualArgc);
    newBuilder.NewArgumentsObj(&argumentsObj, &exit, *argumentsList, actualArgc);
    Bind(&exit);
    Return(*argumentsObj);
}

void GetCallSpreadArgsStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef array = TaggedArgument(1);
    Return(GetCallSpreadArgs(glue, array, ProfileOperation()));
}

void GetPropertyByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    Return(GetPropertyByIndex(glue, receiver, index, ProfileOperation()));
}

void SetPropertyByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    Return(SetPropertyByIndex(glue, receiver, index, value, false));
}

void SetPropertyByIndexWithOwnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    Return(SetPropertyByIndex(glue, receiver, index, value, true));
}

void JSTaggedValueHasPropertyStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2);      // 2 : 3rd para
    Return(HasProperty(glue, obj, key));
}

void GetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef id = Int64Argument(2); // 2 : 3rd para
    GateRef jsFunc = TaggedArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4); // 4 : 5th para
    AccessObjectStubBuilder builder(this, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.LoadObjByName(glue, receiver, id, info, profileTypeInfo, slotId, ProfileOperation()));
}

void DeprecatedGetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedPointerArgument(2); // 2 : 3rd para
    AccessObjectStubBuilder builder(this);
    Return(builder.DeprecatedLoadObjByName(glue, receiver, key));
}

void SetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef id = Int64Argument(2); // 2 : 3rd para
    GateRef value = TaggedPointerArgument(3); // 3 : 4th para
    GateRef jsFunc = TaggedArgument(4); // 4 : 5th para
    GateRef slotId = Int32Argument(5); // 5 : 6th para
    AccessObjectStubBuilder builder(this, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.StoreObjByName(glue, receiver, id, info, value, profileTypeInfo, slotId, ProfileOperation()));
}

void GetPropertyByNameWithMegaStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef megaStubCache = PtrArgument(3);
    GateRef prop = TaggedArgument(4);
    GateRef jsFunc = TaggedArgument(5); // 5 : 6th para
    GateRef slotId = Int32Argument(6); // 6 : 7th para
    AccessObjectStubBuilder builder(this, jsFunc);
    Return(builder.LoadObjByNameWithMega(glue, receiver, megaStubCache, prop, jsFunc, slotId,
                                         ProfileOperation()));
}

void SetPropertyByNameWithMegaStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef value = TaggedPointerArgument(3); // 3 : 4th para
    GateRef megaStubCache = PtrArgument(4); // 4 : 5th para
    GateRef prop = TaggedArgument(5); // 5 : 6th para
    GateRef jsFunc = TaggedArgument(6); // 6 : 7th para
    GateRef slotId = Int32Argument(7); // 7 : 8th para
    AccessObjectStubBuilder builder(this, jsFunc);
    Return(builder.StoreObjByNameWithMega(glue, receiver, value, megaStubCache, prop, jsFunc, slotId,
                                          ProfileOperation()));
}

void DeprecatedSetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    Return(SetPropertyByName(glue, receiver, key, value, false, True()));
}

void SetPropertyByNameWithOwnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    Return(SetPropertyByName(glue, receiver, key, value, true, True()));
}

void GetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef jsFunc = TaggedArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4); // 4 : 5th para
    AccessObjectStubBuilder builder(this);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.LoadObjByValue(glue, receiver, key, profileTypeInfo, slotId));
}

void DeprecatedGetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    Return(GetPropertyByValue(glue, receiver, key, ProfileOperation()));
}

void SetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);        // 2 : 3rd para
    GateRef value = TaggedArgument(3);      // 3 : 4th para
    GateRef jsFunc = TaggedArgument(4);     // 4 : 5th para
    GateRef slotId = Int32Argument(5);      // 5 : 6th para
    AccessObjectStubBuilder builder(this);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.StoreObjByValue(glue, receiver, key, value, profileTypeInfo, slotId));
}

void DeprecatedSetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    Return(SetPropertyByValue(glue, receiver, key, value, false));
}

void SetPropertyByValueWithOwnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    Return(SetPropertyByValue(glue, receiver, key, value, true));
}

void StOwnByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    AccessObjectStubBuilder builder(this);
    Return(builder.StOwnByIndex(glue, receiver, index, value));
}

void StOwnByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    AccessObjectStubBuilder builder(this);
    Return(builder.StOwnByValue(glue, receiver, key, value));
}

void StOwnByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    AccessObjectStubBuilder builder(this);
    Return(builder.StOwnByName(glue, receiver, key, value));
}

void StOwnByValueWithNameSetStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    AccessObjectStubBuilder builder(this);
    Return(builder.StOwnByValueWithNameSet(glue, receiver, key, value));
}

void StOwnByNameWithNameSetStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    AccessObjectStubBuilder builder(this);
    Return(builder.StOwnByNameWithNameSet(glue, receiver, key, value));
}

void LdObjByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    AccessObjectStubBuilder builder(this);
    Return(builder.LdObjByIndex(glue, receiver, index));
}

void StObjByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    AccessObjectStubBuilder builder(this);
    Return(builder.StObjByIndex(glue, receiver, index, value));
}

void TryLdGlobalByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef jsFunc = TaggedArgument(2); // 2 : 3th para
    GateRef slotId = Int32Argument(3); // 3 : 4th para
    AccessObjectStubBuilder builder(this, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.TryLoadGlobalByName(glue, id, info, profileTypeInfo, slotId, ProfileOperation()));
}

void TryStGlobalByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef value = TaggedArgument(2); // 2 : 3rd para
    GateRef jsFunc = TaggedArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4);  // 4: 5th para
    AccessObjectStubBuilder builder(this, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.TryStoreGlobalByName(glue, id, info, value, profileTypeInfo, slotId, ProfileOperation()));
}

void LdGlobalVarStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef jsFunc = TaggedArgument(2); // 2 : 3th para
    GateRef slotId = Int32Argument(3); // 3 : 4th para
    AccessObjectStubBuilder builder(this, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.LoadGlobalVar(glue, id, info, profileTypeInfo, slotId, ProfileOperation()));
}

void StGlobalVarStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef value = TaggedArgument(2); // 2 : 3rd para
    GateRef jsFunc = TaggedArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4);  // 4: 5th para
    AccessObjectStubBuilder builder(this, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.StoreGlobalVar(glue, id, info, value, profileTypeInfo, slotId));
}

void TryLoadICByNameStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef firstValue = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef secondValue = TaggedArgument(3); /* 3 : 4th parameter is value */

    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label cachedHandlerNotHole(env);
    BRANCH(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        {
            Return(LoadICWithHandler(glue, receiver, receiver, secondValue, ProfileOperation()));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(firstValue, hclass);
            BRANCH(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
            Bind(&cachedHandlerNotHole);
            {
                Return(LoadICWithHandler(glue, receiver, receiver, cachedHandler, ProfileOperation()));
            }
        }
    }
    Bind(&receiverNotHeapObject);
    {
        Return(Hole());
    }
}

void TryLoadICByValueStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef firstValue = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef secondValue = TaggedArgument(4); /* 4 : 5th parameter is value */

    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label firstValueEqualKey(env);
    Label cachedHandlerNotHole(env);
    BRANCH(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        Return(LoadElement(glue, receiver, key));
        Bind(&hclassNotEqualFirstValue);
        {
            BRANCH(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                auto cachedHandler = CheckPolyHClass(secondValue, hclass);
                BRANCH(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
                Bind(&cachedHandlerNotHole);
                Return(LoadICWithHandler(glue, receiver, receiver, cachedHandler, ProfileOperation()));
            }
        }
    }
    Bind(&receiverNotHeapObject);
    Return(Hole());
}

void TryStoreICByNameStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef firstValue = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef secondValue = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef value = TaggedArgument(4); /* 4 : 5th parameter is value */
    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label cachedHandlerNotHole(env);
    BRANCH(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        {
            Return(StoreICWithHandler(glue, receiver, receiver, value, secondValue));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(firstValue, hclass);
            BRANCH(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
            Bind(&cachedHandlerNotHole);
            {
                Return(StoreICWithHandler(glue, receiver, receiver, value, cachedHandler));
            }
        }
    }
    Bind(&receiverNotHeapObject);
    Return(Hole());
}

void TryStoreICByValueStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef firstValue = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef secondValue = TaggedArgument(4); /* 4 : 5th parameter is value */
    GateRef value = TaggedArgument(5); /* 5 : 6th parameter is value */
    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label firstValueEqualKey(env);
    Label cachedHandlerNotHole(env);
    BRANCH(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        Return(ICStoreElement(glue, receiver, key, value, secondValue));
        Bind(&hclassNotEqualFirstValue);
        {
            BRANCH(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                GateRef cachedHandler = CheckPolyHClass(secondValue, hclass);
                BRANCH(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
                Bind(&cachedHandlerNotHole);
                Return(StoreICWithHandler(glue, receiver, receiver, value, cachedHandler));
            }
        }
    }
    Bind(&receiverNotHeapObject);
    Return(Hole());
}

void SetValueWithBarrierStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef offset = PtrArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    SetValueWithBarrier(glue, obj, offset, value);
    Return();
}

void SetNonSValueWithBarrierStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef offset = PtrArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    SetValueWithBarrier(glue, obj, offset, value, MemoryAttribute::NON_SHARE);
    Return();
}

void SetSValueWithBarrierStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef offset = PtrArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    SetValueWithBarrier(glue, obj, offset, value, MemoryAttribute::SHARED);
    Return();
}

void VerifyBarrierStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef offset = PtrArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    VerifyBarrier(glue, obj, offset, value);
    Return();
}

void NewThisObjectCheckedStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef ctor = TaggedArgument(1);
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.NewThisObjectChecked(glue, ctor));
}

void ConstructorCheckStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef ctor = TaggedArgument(1);
    GateRef value = TaggedArgument(2); // 2 : 3rd para
    GateRef thisObj = TaggedArgument(3); // 3 : 4th para
    Return(ConstructorCheck(glue, ctor, value, thisObj));
}

void CreateEmptyArrayStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.CreateEmptyArray(glue));
}

void CreateArrayWithBufferStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef index = Int32Argument(1);
    GateRef jsFunc = TaggedArgument(2); // 2 : 3rd para
    GateRef slotId = Int32Argument(5); // 5 : 6th para
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.CreateArrayWithBuffer(
        glue, index, jsFunc, { IntPtr(0), 0, true }, Undefined(), slotId, ProfileOperation()));
}

void NewJSObjectStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef ctor = TaggedArgument(1);
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.FastNewThisObject(glue, ctor));
}

void JsBoundCallInternalStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);
    Label fastCall(env);
    Label notFastCall(env);
    Label methodIsFastCall(env);
    Label fastCallBridge(env);
    Label slowCall(env);
    Label slowCallBridge(env);

    GateRef glue = PtrArgument(0);
    GateRef argc = Int64Argument(1);
    GateRef func = TaggedPointerArgument(2); // callTarget
    GateRef argv = PtrArgument(3);
    GateRef thisValue = TaggedPointerArgument(4); // this
    GateRef newTarget = TaggedPointerArgument(5); // new target
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef method = GetMethodFromFunction(func);
    GateRef callfield = Load(VariableType::INT64(), method, IntPtr(Method::CALL_FIELD_OFFSET));
    GateRef expectedNum = Int64And(Int64LSR(callfield, Int64(MethodLiteral::NumArgsBits::START_BIT)),
        Int64((1LU << MethodLiteral::NumArgsBits::SIZE) - 1));
    GateRef expectedArgc = Int64Add(expectedNum, Int64(NUM_MANDATORY_JSFUNC_ARGS));
    GateRef actualArgc = Int64Sub(argc, IntPtr(NUM_MANDATORY_JSFUNC_ARGS));
    BRANCH(JudgeAotAndFastCall(func, CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL),
        &methodIsFastCall, &notFastCall);
    Bind(&methodIsFastCall);
    {
        BRANCH(Int64Equal(expectedArgc, argc), &fastCall, &fastCallBridge);
        Bind(&fastCall);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgV),
                { glue, func, thisValue, actualArgc, argv });
            Jump(&exit);
        }
        Bind(&fastCallBridge);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgVAndPushArgv),
                { glue, func, thisValue, actualArgc, argv, expectedNum });
            Jump(&exit);
        }
    }
    Bind(&notFastCall);
    {
        BRANCH(Int64Equal(expectedArgc, argc), &slowCall, &slowCallBridge);
        Bind(&slowCall);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgV),
                { glue, actualArgc, func, newTarget, thisValue, argv });
            Jump(&exit);
        }
        Bind(&slowCallBridge);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgVAndPushArgv),
                { glue, actualArgc, func, newTarget, thisValue, argv });
            Jump(&exit);
        }
    }
    Bind(&exit);
    Return(*result);
}

void GetSingleCharCodeByIndexStubBuilder::GenerateCircuit()
{
    GateRef str = TaggedArgument(1);
    GateRef index = Int32Argument(2);
    BuiltinsStringStubBuilder builder(this);
    GateRef result = builder.GetSingleCharCodeByIndex(str, index);
    Return(result);
}

void CreateStringBySingleCharCodeStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef charCode = Int32Argument(1);
    BuiltinsStringStubBuilder builder(this);
    GateRef result = builder.CreateStringBySingleCharCode(glue, charCode);
    Return(result);
}

void FastStringEqualStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef str1 = TaggedArgument(1);
    GateRef str2 = Int32Argument(2);

    Label leftEqualRight(env);
    Label leftNotEqualRight(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    BRANCH(Equal(str1, str2), &leftEqualRight, &leftNotEqualRight);
    Bind(&leftEqualRight);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&leftNotEqualRight);
    {
        result = FastStringEqual(glue, str1, str2);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*result);
}

void FastStringAddStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef str1 = TaggedArgument(1);
    GateRef str2 = Int32Argument(2);

    BuiltinsStringStubBuilder builtinsStringStubBuilder(this);
    GateRef result = builtinsStringStubBuilder.StringConcat(glue, str1, str2);
    Return(result);
}

void StringAddStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef str1 = TaggedArgument(1);
    GateRef str2 = TaggedArgument(2);       // 2: 3rd argument
    GateRef status = Int32Argument(3);      // 3: 4th argument

    BuiltinsStringStubBuilder builtinsStringStubBuilder(this);
    GateRef result = builtinsStringStubBuilder.StringAdd(glue, str1, str2, status);
    Return(result);
}

void DeleteObjectPropertyStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef object = TaggedArgument(1);
    GateRef prop = TaggedArgument(2);
    GateRef result = DeletePropertyOrThrow(glue, object, prop);
    Return(result);
}

void GetpropiteratorStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef object = TaggedArgument(1);
    NewObjectStubBuilder newBuilder(this);
    GateRef result = newBuilder.EnumerateObjectProperties(glue, object);
    Return(result);
}

void GetnextpropnameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef iter = TaggedArgument(1);
    GateRef result = NextInternal(glue, iter);
    Return(result);
}

#define CREATE_ITERATOR_STUB_BUILDER(name, collection, iterationKind)                                            \
void name##StubBuilder::GenerateCircuit()                                                                        \
{                                                                                                                \
    auto env = GetEnvironment();                                                                                 \
    Label exit(env);                                                                                             \
                                                                                                                 \
    GateRef glue = PtrArgument(0);                                                                               \
    GateRef obj = TaggedArgument(1);                                                                             \
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());                                                    \
                                                                                                                 \
    NewObjectStubBuilder newBuilder(this);                                                                       \
    newBuilder.SetGlue(glue);                                                                                    \
    GateRef kind = Int32(static_cast<int32_t>(IterationKind::iterationKind));                                    \
    newBuilder.CreateJSCollectionIterator<JS##collection##Iterator, JS##collection>(&result, &exit, obj, kind);  \
    Bind(&exit);                                                                                                 \
    Return(*result);                                                                                             \
}

CREATE_ITERATOR_STUB_BUILDER(CreateJSSetIterator, Set, VALUE)
CREATE_ITERATOR_STUB_BUILDER(JSSetEntries, Set, KEY_AND_VALUE)
CREATE_ITERATOR_STUB_BUILDER(JSMapKeys, Map, KEY)
CREATE_ITERATOR_STUB_BUILDER(JSMapValues, Map, VALUE)
CREATE_ITERATOR_STUB_BUILDER(CreateJSMapIterator, Map, KEY_AND_VALUE)

void StringIteratorNextStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);
    Label slowpath(env);

    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    BuiltinsStringStubBuilder builder(this);
    builder.StringIteratorNext(glue, obj, Gate::InvalidGateRef, &result, &exit, &slowpath);
    Bind(&slowpath);
    {
        result = CallRuntime(glue, RTSTUB_ID(StringIteratorNext), { obj });
        Jump(&exit);
    }
    Bind(&exit);
    Return(*result);
}

void ArrayIteratorNextStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);
    Label slowpath(env);

    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    BuiltinsArrayStubBuilder builder(this);
    builder.ArrayIteratorNext(glue, obj, Gate::InvalidGateRef, &result, &exit, &slowpath);
    Bind(&slowpath);
    {
        result = CallRuntime(glue, RTSTUB_ID(ArrayIteratorNext), { obj });
        Jump(&exit);
    }
    Bind(&exit);
    Return(*result);
}

void MapIteratorNextStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);

    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    BuiltinsCollectionIteratorStubBuilder<JSMapIterator> builder(this, glue, obj, Gate::InvalidGateRef);
    builder.Next(&result, &exit);
    Bind(&exit);
    Return(*result);
}

void SetIteratorNextStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);

    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    BuiltinsCollectionIteratorStubBuilder<JSSetIterator> builder(this, glue, obj, Gate::InvalidGateRef);
    builder.Next(&result, &exit);
    Bind(&exit);
    Return(*result);
}

void GetIteratorStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);

    GateRef res = GetIterator(glue, obj, ProfileOperation());
    Return(res);
}

void JSMapGetStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);

    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> builder(this, glue);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Get(linkedTable, key));
}

void JSMapHasStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);

    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> builder(this, glue);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Has(linkedTable, key));
}

void JSSetHasStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);

    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> builder(this, glue);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Has(linkedTable, key));
}

void CreateJSTypedArrayEntriesStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);

    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetGlue(glue);
    GateRef kind = Int32(static_cast<int32_t>(IterationKind::KEY_AND_VALUE));
    newBuilder.CreateJSTypedArrayIterator(&result, &exit, obj, kind);
    Bind(&exit);
    Return(*result);
}

void CreateJSTypedArrayKeysStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);

    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetGlue(glue);
    GateRef kind = Int32(static_cast<int32_t>(IterationKind::KEY));
    newBuilder.CreateJSTypedArrayIterator(&result, &exit, obj, kind);
    Bind(&exit);
    Return(*result);
}

void CreateJSTypedArrayValuesStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);

    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetGlue(glue);
    GateRef kind = Int32(static_cast<int32_t>(IterationKind::VALUE));
    newBuilder.CreateJSTypedArrayIterator(&result, &exit, obj, kind);
    Bind(&exit);
    Return(*result);
}

void JSMapDeleteStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);

    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> builder(this, glue);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Delete(linkedTable, key));
}

void JSSetDeleteStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);

    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> builder(this, glue);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Delete(linkedTable, key));
}

void JSSetAddStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);

    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> builder(this, glue);
    GateRef linkedTable = builder.GetLinked(obj);
    GateRef newTable = builder.Insert(linkedTable, key, key);
    builder.Store(VariableType::JS_ANY(), glue, obj, IntPtr(JSSet::LINKED_SET_OFFSET), newTable);
    Return(obj);
}

void GrowElementsCapacityStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef thisValue = TaggedArgument(1);
    GateRef newLength = Int32Argument(2U);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    BuiltinsArrayStubBuilder builder(this);
    result = builder.GrowElementsCapacity(glue, thisValue, newLength);
    Return(*result);
}

void SameValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef left = TaggedArgument(1);
    GateRef right = TaggedArgument(2U);
    GateRef result = SameValue(glue, left, right);
    Return(result);
}

void BatchBarrierStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef dstObj = PtrArgument(1);
    GateRef dstAddr = PtrArgument(2);
    GateRef taggedValueCount = TaggedArgument(3);
    BarrierStubBuilder barrierBuilder(this, glue, dstObj, dstAddr, taggedValueCount);
    barrierBuilder.DoBatchBarrier();
    Return();
}

void MoveBarrierInRegionStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef dstObj = PtrArgument(1);
    GateRef dstAddr = PtrArgument(2);
    GateRef count = Int32Argument(3);
    GateRef srcAddr = PtrArgument(4);
    BarrierStubBuilder barrierBuilder(this, glue, dstObj, dstAddr, count);
    barrierBuilder.DoMoveBarrierInRegion(srcAddr);
    Return();
}

void MoveBarrierCrossRegionStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef dstObj = PtrArgument(1);
    GateRef dstAddr = PtrArgument(2);
    GateRef count = Int32Argument(3);
    GateRef srcAddr = PtrArgument(4);
    GateRef srcObj = PtrArgument(5);
    BarrierStubBuilder barrierBuilder(this, glue, dstObj, dstAddr, count);
    barrierBuilder.DoMoveBarrierCrossRegion(srcAddr, srcObj);
    Return();
}

void FindEntryFromNameDictionaryStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef taggedArray = PtrArgument(1);
    GateRef key = PtrArgument(2);
    GateRef entry = FindEntryFromNameDictionary(glue, taggedArray, key);
    Return(entry);
}

CallSignature CommonStubCSigns::callSigns_[CommonStubCSigns::NUM_OF_STUBS];

void CommonStubCSigns::Initialize()
{
#define INIT_SIGNATURES(name)                                                              \
    name##CallSignature::Initialize(&callSigns_[name]);                                    \
    callSigns_[name].SetID(name);                                                          \
    callSigns_[name].SetName(std::string("COStub_") + #name);                              \
    callSigns_[name].SetConstructor(                                                       \
        [](void* env) {                                                                    \
            return static_cast<void*>(                                                     \
                new name##StubBuilder(&callSigns_[name], static_cast<Environment*>(env))); \
        });

    COMMON_STUB_ID_LIST(INIT_SIGNATURES)
#undef INIT_SIGNATURES
}

void CommonStubCSigns::GetCSigns(std::vector<const CallSignature*>& outCSigns)
{
    for (size_t i = 0; i < NUM_OF_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}
}  // namespace panda::ecmascript::kungfu
