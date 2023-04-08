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
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    Return(InstanceOf(glue, x, y));
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
    Return(GetPropertyByIndex(glue, receiver, index));
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
    GateRef prop = TaggedPointerArgument(2); // 2 : 3rd para
    GateRef profileTypeInfo = TaggedPointerArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4); // 4 : 5th para
    AccessObjectStubBuilder builder(this);
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
    Return(builder.LoadObjByName(glue, receiver, prop, info, profileTypeInfo, slotId));
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
    GateRef prop = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedPointerArgument(3); // 3 : 4th para
    GateRef profileTypeInfo = TaggedPointerArgument(4); // 4 : 5th para
    GateRef slotId = Int32Argument(5); // 5 : 6th para
    AccessObjectStubBuilder builder(this);
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
    Return(builder.StoreObjByName(glue, receiver, prop, info, value, profileTypeInfo, slotId));
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
    GateRef profileTypeInfo = TaggedPointerArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4); // 4 : 5th para
    AccessObjectStubBuilder builder(this);
    Return(builder.LoadObjByValue(glue, receiver, key, profileTypeInfo, slotId));
}

void DeprecatedGetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    Return(GetPropertyByValue(glue, receiver, key));
}

void SetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    GateRef slotId = Int32Argument(5); /* 5 : 6th parameter is slotId */
    AccessObjectStubBuilder builder(this);
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
    GateRef prop = TaggedPointerArgument(1);
    GateRef profileTypeInfo = TaggedPointerArgument(2); // 2 : 3rd para
    GateRef slotId = Int32Argument(3); // 3 : 4th para
    AccessObjectStubBuilder builder(this);
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
    Return(builder.TryLoadGlobalByName(glue, prop, info, profileTypeInfo, slotId));
}

void TryStGlobalByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef prop = TaggedPointerArgument(1);
    GateRef value = TaggedArgument(2); // 2 : 3rd para
    GateRef profileTypeInfo = TaggedPointerArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4);  // 4: 5th para
    AccessObjectStubBuilder builder(this);
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
    Return(builder.TryStoreGlobalByName(glue, prop, info, value, profileTypeInfo, slotId));
}

void LdGlobalVarStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef prop = TaggedPointerArgument(1);
    GateRef profileTypeInfo = TaggedPointerArgument(2); // 2 : 3rd para
    GateRef slotId = Int32Argument(3); // 3 : 4th para
    AccessObjectStubBuilder builder(this);
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
    Return(builder.LoadGlobalVar(glue, prop, info, profileTypeInfo, slotId));
}

void StGlobalVarStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef prop = TaggedPointerArgument(1);
    GateRef value = TaggedArgument(2); // 2 : 3rd para
    GateRef profileTypeInfo = TaggedPointerArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4);  // 4: 5th para
    AccessObjectStubBuilder builder(this);
    StringIdInfo info = { 0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID };
    Return(builder.StoreGlobalVar(glue, prop, info, value, profileTypeInfo, slotId));
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
            Return(LoadICWithHandler(glue, receiver, receiver, secondValue));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(firstValue, hclass);
            Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
            Bind(&cachedHandlerNotHole);
            {
                Return(LoadICWithHandler(glue, receiver, receiver, cachedHandler));
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
        Return(LoadElement(receiver, key));
        Bind(&hclassNotEqualFirstValue);
        {
            Branch(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                auto cachedHandler = CheckPolyHClass(secondValue, hclass);
                Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
                Bind(&cachedHandlerNotHole);
                Return(LoadICWithHandler(glue, receiver, receiver, cachedHandler));
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
        Return(ICStoreElement(glue, receiver, key, value, secondValue));
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
        GateRef globalConstOffset = IntPtr(JSThread::GlueData::GetGlobalConstOffset(env->Is32Bit()));
        GateRef keyOffset = PtrAdd(globalConstOffset,
            PtrMul(IntPtr(static_cast<int64_t>(ConstantIndex::APPLY_STRING_INDEX)),
            IntPtr(sizeof(JSTaggedValue))));
        GateRef key = Load(VariableType::JS_ANY(), glue, keyOffset);
        GateRef method = CallRuntime(glue, RTSTUB_ID(JSObjectGetMethod), {handler, key});
        ReturnExceptionIfAbruptCompletion(glue);

        Branch(TaggedIsUndefined(method), &isUndefined, &isNotUndefined);
        Bind(&isUndefined);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSProxyCallInternalWithArgV), {glue, argc, target, argv});
            Return(*result);
        }
        Bind(&isNotUndefined);
        {
            const int JSPROXY_NUM_ARGS = 3;
            GateRef arrHandle = CallRuntime(glue, RTSTUB_ID(CreateArrayFromList), argc, argv);
            // 2: this offset
            GateRef thisArg = Load(VariableType::JS_POINTER(), argv, IntPtr(2 * sizeof(JSTaggedValue)));
            GateRef numArgs = Int64(JSPROXY_NUM_ARGS + NUM_MANDATORY_JSFUNC_ARGS);
            GateRef lexEnv = Load(VariableType::JS_POINTER(), method, IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
            result = CallNGCRuntime(glue, RTSTUB_ID(JSCall),
                {glue, lexEnv, numArgs, method, Undefined(), handler, target, thisArg, arrHandle});
            Jump(&exit);
        }
    }
    Bind(&exit);
    Return(*result);
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
