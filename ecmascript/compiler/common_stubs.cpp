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
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/js_array.h"
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

void ToBooleanStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    Return(FastToBoolean(x));
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
    Branch(TaggedIsException(*argumentsList), &exit, &newArgumentsObj);
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
    Return(SetPropertyByIndex(glue, receiver, index, value, false, ProfileOperation()));
}

void SetPropertyByIndexWithOwnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    Return(SetPropertyByIndex(glue, receiver, index, value, true, ProfileOperation()));
}

void GetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef id = Int64Argument(2); // 2 : 3rd para
    GateRef jsFunc = TaggedArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4); // 4 : 5th para
    AccessObjectStubBuilder builder(this, jsFunc);
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
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
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.StoreObjByName(glue, receiver, id, info, value, profileTypeInfo, slotId, ProfileOperation()));
}

void DeprecatedSetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    Return(SetPropertyByName(glue, receiver, key, value, false));
}

void SetPropertyByNameWithOwnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    Return(SetPropertyByName(glue, receiver, key, value, true));
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

void TryLdGlobalByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef jsFunc = TaggedArgument(2); // 2 : 3th para
    GateRef slotId = Int32Argument(3); // 3 : 4th para
    AccessObjectStubBuilder builder(this, jsFunc);
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
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
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
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
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
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
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
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
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        Branch(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        {
            Return(LoadICWithHandler(glue, receiver, receiver, secondValue, ProfileOperation()));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(firstValue, hclass);
            Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
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
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        Branch(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        Return(LoadElement(glue, receiver, key, ProfileOperation()));
        Bind(&hclassNotEqualFirstValue);
        {
            Branch(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                auto cachedHandler = CheckPolyHClass(secondValue, hclass);
                Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
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
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        Branch(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        {
            Return(StoreICWithHandler(glue, receiver, receiver, value, secondValue));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(firstValue, hclass);
            Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
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
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        Branch(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        Return(ICStoreElement(glue, receiver, key, value, secondValue, ProfileOperation()));
        Bind(&hclassNotEqualFirstValue);
        {
            Branch(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                GateRef cachedHandler = CheckPolyHClass(secondValue, hclass);
                Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
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
    Return(newBuilder.CreateEmptyArray(glue, Undefined(), Undefined(),
        Undefined(), Undefined(), ProfileOperation()));
}

void CreateArrayWithBufferStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef index = Int32Argument(1);
    GateRef jsFunc = TaggedArgument(2); // 2 : 3rd para
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.CreateArrayWithBuffer(glue, index, jsFunc, Undefined(),
        Undefined(), Undefined(), ProfileOperation()));
}

void NewJSObjectStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef hclass = TaggedArgument(1);
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.NewJSObject(glue, hclass));
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
    GateRef hClass = LoadHClass(func);
    GateRef bitfield = Load(VariableType::INT32(), hClass, Int32(JSHClass::BIT_FIELD_OFFSET));
    Branch(CanFastCallWithBitField(bitfield), &methodIsFastCall, &notFastCall);
    Bind(&methodIsFastCall);
    {
        Branch(Int64LessThanOrEqual(expectedArgc, argc), &fastCall, &fastCallBridge);
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
        Branch(Int64LessThanOrEqual(expectedArgc, argc), &slowCall, &slowCallBridge);
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
    Branch(TaggedIsNull(handler), &isNull, &notNull);
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

        Branch(TaggedIsUndefined(method), &isUndefined, &isNotUndefined);
        Bind(&isUndefined);
        {
            Label isHeapObject(env);
            Label slowPath(env);
            Label isJsFcuntion(env);
            Label notCallConstructor(env);
            Label fastCall(env);
            Label notFastCall(env);
            Label slowCall(env);
            Branch(TaggedIsHeapObject(target), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                GateRef hClass = LoadHClass(target);
                GateRef bitfield = Load(VariableType::INT32(), hClass, Int32(JSHClass::BIT_FIELD_OFFSET));
                Branch(IsClassConstructorFromBitField(bitfield), &slowPath, &notCallConstructor);
                Bind(&notCallConstructor);
                GateRef actualArgc = Int64Sub(argc, IntPtr(NUM_MANDATORY_JSFUNC_ARGS));
                GateRef actualArgv = PtrAdd(argv, IntPtr(NUM_MANDATORY_JSFUNC_ARGS * sizeof(JSTaggedValue)));
                Branch(CanFastCallWithBitField(bitfield), &fastCall, &notFastCall);
                Bind(&fastCall);
                {
                    result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgV),
                        { glue, target, thisTarget, actualArgc, actualArgv });
                    Jump(&exit);
                }
                Bind(&notFastCall);
                {
                    Branch(IsOptimizedWithBitField(bitfield), &slowCall, &slowPath);
                    Bind(&slowCall);
                    {
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgV),
                            { glue, actualArgc, target, newTarget, thisTarget, actualArgv });
                        Jump(&exit);
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

            Branch(TaggedIsHeapObject(method), &isHeapObject1, &slowPath1);
            Bind(&isHeapObject1);
            {
                GateRef hClass = LoadHClass(method);
                GateRef bitfield = Load(VariableType::INT32(), hClass, Int32(JSHClass::BIT_FIELD_OFFSET));
                Branch(IsClassConstructor(method), &slowPath1, &notCallConstructor1);
                Bind(&notCallConstructor1);
                GateRef meth = GetMethodFromFunction(method);
                GateRef code = GetAotCodeAddr(meth);
                Branch(CanFastCallWithBitField(bitfield), &fastCall1, &notFastCall1);
                Bind(&fastCall1);
                {
                    result = FastCallOptimized(glue, code,
                        { glue, method, handler, target, thisTarget, arrHandle });
                    Jump(&exit);
                }
                Bind(&notFastCall1);
                {
                    Branch(IsOptimizedWithBitField(bitfield), &slowCall1, &slowPath1);
                    Bind(&slowCall1);
                    {
                        result = CallOptimized(glue, code,
                            { glue, numArgs, method, Undefined(), handler, target, thisTarget, arrHandle });
                        Jump(&exit);
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

void GetCharFromEcmaStringStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef str = TaggedArgument(1);
    GateRef index = Int32Argument(2);

    BuiltinsStringStubBuilder builder(this);
    GateRef result = builder.CreateFromEcmaString(glue, str, index);
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
