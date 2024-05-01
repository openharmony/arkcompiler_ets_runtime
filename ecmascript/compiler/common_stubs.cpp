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

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"
#include "ecmascript/compiler/builtins/linked_hashtable_stub_builder.h"
#include "ecmascript/compiler/codegen/llvm/llvm_ir_builder.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table.h"

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

void CopyRestArgsStubBuilder::GenerateCircuit()
{
    DEFVARIABLE(argumentsList, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(arrayObj, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(actualRestNum, VariableType::INT32(), Int32(0));
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef startIdx = Int32Argument(1);
    GateRef numArgs = Int32Argument(2);
    Label afterArgumentsList(env);
    Label newArgumentsObj(env);
    Label numArgsGreater(env);
    Label numArgsNotGreater(env);
    GateRef argv = CallNGCRuntime(glue, RTSTUB_ID(GetActualArgvNoGC), { glue });
    GateRef args = PtrAdd(argv, IntPtr(NUM_MANDATORY_JSFUNC_ARGS * 8)); // 8: ptr size
    GateRef actualArgc = Int32Sub(numArgs, Int32(NUM_MANDATORY_JSFUNC_ARGS));
    // 1. Calculate actual rest num.
    BRANCH(Int32UnsignedGreaterThan(actualArgc, startIdx), &numArgsGreater, &numArgsNotGreater);
    Bind(&numArgsGreater);
    {
        actualRestNum = Int32Sub(actualArgc, startIdx);
        Jump(&numArgsNotGreater);
    }
    Bind(&numArgsNotGreater);
    // 2. Construct arguments list.
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    newBuilder.NewArgumentsList(&argumentsList, &afterArgumentsList, args, startIdx, *actualRestNum);
    Bind(&afterArgumentsList);
    // 3. Construct rest array.
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef hclass = Load(VariableType::JS_POINTER(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    arrayObj = newBuilder.NewJSArrayWithSize(hclass, *actualRestNum);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, *arrayObj, lengthOffset, *actualRestNum);
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    SetPropertyInlinedProps(glue, *arrayObj, hclass, accessor, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
    SetExtensibleToBitfield(glue, *arrayObj, true);
    SetElementsArray(VariableType::JS_POINTER(), glue, *arrayObj, *argumentsList);
    Return(*arrayObj);
}

void GetUnmapedArgsStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef numArgs = Int32Argument(1);

    DEFVARIABLE(argumentsList, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(argumentsObj, VariableType::JS_ANY(), Hole());
    Label afterArgumentsList(env);
    Label newArgumentsObj(env);
    Label exit(env);

    GateRef argv = CallNGCRuntime(glue, RTSTUB_ID(GetActualArgvNoGC), { glue });
    GateRef args = PtrAdd(argv, IntPtr(NUM_MANDATORY_JSFUNC_ARGS * 8)); // 8: ptr size
    GateRef actualArgc = Int32Sub(numArgs, Int32(NUM_MANDATORY_JSFUNC_ARGS));
    GateRef startIdx = Int32(0);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    newBuilder.NewArgumentsList(&argumentsList, &afterArgumentsList, args, startIdx, actualArgc);
    Bind(&afterArgumentsList);
    BRANCH(TaggedIsException(*argumentsList), &exit, &newArgumentsObj);
    Bind(&newArgumentsObj);
    newBuilder.NewArgumentsObj(&argumentsObj, &exit, *argumentsList, actualArgc);
    Bind(&exit);
    Return(*argumentsObj);
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
    BRANCH(JudgeAotAndFastCallWithMethod(method, CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL),
        &methodIsFastCall, &notFastCall);
    Bind(&methodIsFastCall);
    {
        BRANCH(Int64LessThanOrEqual(expectedArgc, argc), &fastCall, &fastCallBridge);
        Bind(&fastCall);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgV),
                { glue, func, thisValue, actualArgc, argv });
            Jump(&exit);
        }
        Bind(&fastCallBridge);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgVAndPushUndefined),
                { glue, func, thisValue, actualArgc, argv, expectedNum });
            Jump(&exit);
        }
    }
    Bind(&notFastCall);
    {
        BRANCH(Int64LessThanOrEqual(expectedArgc, argc), &slowCall, &slowCallBridge);
        Bind(&slowCall);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgV),
                { glue, actualArgc, func, newTarget, thisValue, argv });
            Jump(&exit);
        }
        Bind(&slowCallBridge);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgVAndPushUndefined),
                { glue, actualArgc, func, newTarget, thisValue, argv });
            Jump(&exit);
        }
    }
    Bind(&exit);
    Return(*result);
}

void JsProxyCallInternalStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    Label exit(env);
    Label isNull(env);
    Label notNull(env);
    Label isUndefined(env);
    Label isNotUndefined(env);

    GateRef glue = PtrArgument(0);
    GateRef argc = Int64Argument(1);
    GateRef proxy = TaggedPointerArgument(2); // callTarget
    GateRef argv = PtrArgument(3);
    GateRef newTarget = Load(VariableType::JS_POINTER(), argv, IntPtr(sizeof(JSTaggedValue)));
    GateRef thisTarget = Load(VariableType::JS_POINTER(), argv, IntPtr(2 * sizeof(JSTaggedValue)));

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef handler = GetHandlerFromJSProxy(proxy);
    BRANCH(TaggedIsNull(handler), &isNull, &notNull);
    Bind(&isNull);
    {
        ThrowTypeAndReturn(glue, GET_MESSAGE_STRING_ID(NonCallable), Exception());
    }
    Bind(&notNull);
    {
        GateRef target = GetTargetFromJSProxy(proxy);
        GateRef gConstPointer = Load(VariableType::JS_ANY(), glue,
            IntPtr(JSThread::GlueData::GetGlobalConstOffset(env->Is32Bit())));

        GateRef keyOffset = PtrAdd(gConstPointer,
            PtrMul(IntPtr(static_cast<int64_t>(ConstantIndex::APPLY_STRING_INDEX)),
            IntPtr(sizeof(JSTaggedValue))));
        GateRef key = Load(VariableType::JS_ANY(), keyOffset);
        GateRef method = CallRuntime(glue, RTSTUB_ID(JSObjectGetMethod), {handler, key});
        ReturnExceptionIfAbruptCompletion(glue);

        BRANCH(TaggedIsUndefined(method), &isUndefined, &isNotUndefined);
        Bind(&isUndefined);
        {
            Label isHeapObject(env);
            Label slowPath(env);
            Label isJsFcuntion(env);
            Label notCallConstructor(env);
            Label fastCall(env);
            Label notFastCall(env);
            Label slowCall(env);
            BRANCH(TaggedIsHeapObject(target), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                BRANCH(IsJSFunction(target), &isJsFcuntion, &slowPath);
                Bind(&isJsFcuntion);
                {
                    BRANCH(IsClassConstructor(target), &slowPath, &notCallConstructor);
                    Bind(&notCallConstructor);
                    GateRef meth = GetMethodFromFunction(target);
                    GateRef actualArgc = Int64Sub(argc, IntPtr(NUM_MANDATORY_JSFUNC_ARGS));
                    GateRef actualArgv =
                        PtrAdd(argv, IntPtr(NUM_MANDATORY_JSFUNC_ARGS * JSTaggedValue::TaggedTypeSize()));
                    BRANCH(JudgeAotAndFastCallWithMethod(meth, CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL),
                        &fastCall, &notFastCall);
                    Bind(&fastCall);
                    {
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgV),
                            { glue, target, thisTarget, actualArgc, actualArgv });
                        Jump(&exit);
                    }
                    Bind(&notFastCall);
                    {
                        BRANCH(JudgeAotAndFastCallWithMethod(meth, CircuitBuilder::JudgeMethodType::HAS_AOT),
                            &slowCall, &slowPath);
                        Bind(&slowCall);
                        {
                            result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgV),
                                { glue, actualArgc, target, newTarget, thisTarget, actualArgv });
                            Jump(&exit);
                        }
                    }
                }
            }
            Bind(&slowPath);
            {
                result = CallNGCRuntime(glue, RTSTUB_ID(JSProxyCallInternalWithArgV), {glue, target});
                Jump(&exit);
            }
        }
        Bind(&isNotUndefined);
        {
            Label isHeapObject1(env);
            Label slowPath1(env);
            Label isJsFcuntion1(env);
            Label notCallConstructor1(env);
            Label fastCall1(env);
            Label notFastCall1(env);
            Label slowCall1(env);
            const int JSPROXY_NUM_ARGS = 3;
            GateRef arrHandle = CallRuntime(glue, RTSTUB_ID(CreateArrayFromList), argc, argv);
            // 2: this offset
            GateRef numArgs = Int64(JSPROXY_NUM_ARGS + NUM_MANDATORY_JSFUNC_ARGS);

            BRANCH(TaggedIsHeapObject(method), &isHeapObject1, &slowPath1);
            Bind(&isHeapObject1);
            {
                BRANCH(IsJSFunction(method), &isJsFcuntion1, &slowPath1);
                Bind(&isJsFcuntion1);
                {
                    BRANCH(IsClassConstructor(method), &slowPath1, &notCallConstructor1);
                    Bind(&notCallConstructor1);
                    GateRef meth = GetMethodFromFunction(method);
                    GateRef code = GetAotCodeAddr(method);
                    BRANCH(JudgeAotAndFastCallWithMethod(meth, CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL),
                        &fastCall1, &notFastCall1);
                    Bind(&fastCall1);
                    {
                        result = FastCallOptimized(glue, code,
                            { glue, method, handler, target, thisTarget, arrHandle });
                        Jump(&exit);
                    }
                    Bind(&notFastCall1);
                    {
                        BRANCH(JudgeAotAndFastCallWithMethod(meth, CircuitBuilder::JudgeMethodType::HAS_AOT),
                            &slowCall1, &slowPath1);
                        Bind(&slowCall1);
                        {
                            result = CallOptimized(glue, code,
                                { glue, numArgs, method, Undefined(), handler, target, thisTarget, arrHandle });
                            Jump(&exit);
                        }
                    }
                }
            }
            Bind(&slowPath1);
            {
                result = CallNGCRuntime(glue, RTSTUB_ID(JSCall),
                    { glue, numArgs, method, Undefined(), handler, target, thisTarget, arrHandle });
                Jump(&exit);
            }
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

void SameValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef left = TaggedArgument(1);
    GateRef right = TaggedArgument(2U);
    GateRef result = SameValue(glue, left, right);
    Return(result);
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
