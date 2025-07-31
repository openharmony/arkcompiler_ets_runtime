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
#include "ecmascript/compiler/builtins/builtins_proxy_stub_builder.h"
#include "ecmascript/compiler/builtins/linked_hashtable_stub_builder.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/global_env_constants.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

void AddStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Add(glue, x, y));
}

void SubStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    GateRef globalEnv = TaggedArgument(3); // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Sub(glue, x, y));
}

void DefineFieldStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef propKey = TaggedArgument(2);    // 2: 3rd argument
    GateRef acc = TaggedArgument(3);        // 3: 4th argument
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th argument
    SetCurrentGlobalEnv(globalEnv);
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
    GateRef globalEnv = TaggedArgument(6); // 6: 7th argument

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label failed(env);
    NewObjectStubBuilder newBuilder(this, globalEnv);
    newBuilder.NewJSFunction(glue, jsFunc, methodId, length, lexEnv, &result, &exit, &failed, slotId);
    Bind(&failed);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    Return(*result);
}

#define JIT_DEFINEFUNC_STUB_GENERATOR(Name, kind)                                                                      \
    void Define##Name##ForJitStubBuilder::GenerateCircuit()                                                            \
    {                                                                                                                  \
        auto env = GetEnvironment();                                                                                   \
        GateRef glue = PtrArgument(0);                                                                                 \
        GateRef jsFunc = TaggedArgument(1);                                                                            \
        GateRef hclass = TaggedArgument(2); /* 2: 3rd argument */                                                      \
        GateRef method = TaggedArgument(3); /* 3: 4th argument */                                                      \
        GateRef length = Int32Argument(4);  /* 4: 5th argument */                                                      \
        GateRef lexEnv = TaggedArgument(5); /* 5: 6th argument */                                                      \
        GateRef slotId = Int32Argument(6);  /* 6: 7th argument */                                                      \
        DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());                                                      \
        Label exit(env);                                                                                               \
        Label failed(env);                                                                                             \
        NewObjectStubBuilder newBuilder(this);                                                                         \
        newBuilder.NewJSFunctionForJit(glue, jsFunc, hclass, method, length, lexEnv, &result, &exit, &failed, slotId,  \
                                       kind);                                                                          \
        Bind(&failed);                                                                                                 \
        {                                                                                                              \
            result = Exception();                                                                                      \
            Jump(&exit);                                                                                               \
        }                                                                                                              \
        Bind(&exit);                                                                                                   \
        Return(*result);                                                                                               \
    }

JIT_DEFINEFUNC_STUB_GENERATOR(NormalFunc, FunctionKind::NORMAL_FUNCTION)
JIT_DEFINEFUNC_STUB_GENERATOR(ArrowFunc, FunctionKind::ARROW_FUNCTION)
JIT_DEFINEFUNC_STUB_GENERATOR(BaseConstructor, FunctionKind::BASE_CONSTRUCTOR)

void CallArg0StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLARG0_IMM8);
    Return(callBuilder.CallStubDispatch(!GetCallSignature()->IsStwCopyStub()));
}

void CallArg1StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLARG1_IMM8_V8);
    Return(callBuilder.CallStubDispatch(!GetCallSignature()->IsStwCopyStub()));
}

void CallArg2StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLARGS2_IMM8_V8_V8);
    Return(callBuilder.CallStubDispatch(!GetCallSignature()->IsStwCopyStub()));
}

void CallArg3StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8);
    Return(callBuilder.CallStubDispatch(!GetCallSignature()->IsStwCopyStub()));
}

void CallThis0StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLTHIS0_IMM8_V8);
    Return(callBuilder.CallStubDispatch(!GetCallSignature()->IsStwCopyStub()));
}

void CallThis1StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLTHIS1_IMM8_V8_V8);
    Return(callBuilder.CallStubDispatch(!GetCallSignature()->IsStwCopyStub()));
}

void CallThis2StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8);
    Return(callBuilder.CallStubDispatch(!GetCallSignature()->IsStwCopyStub()));
}

void CallThis3StubStubBuilder::GenerateCircuit()
{
    CallCoStubBuilder callBuilder(this, EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8);
    Return(callBuilder.CallStubDispatch(!GetCallSignature()->IsStwCopyStub()));
}

void NewFloat32ArrayWithNoArgsStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef globalEnv = TaggedArgument(1);  // 1: global env
    NewObjectStubBuilder objBuilder(env, globalEnv);
    objBuilder.SetParameters(glue, 0);
    GateRef res = objBuilder.NewFloat32ArrayWithSize(glue, Int32(0));
    Return(res);
}

void NewFloat32ArrayStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef ctor = TaggedArgument(1);
    GateRef arg0 = TaggedArgument(2); /* 2 : length */
    GateRef globalEnv = TaggedArgument(3); /* 3 : global env */

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label slowPath(env);
    Label exit(env);
    DEFVALUE(arrayLength, (env), VariableType::INT64(), Int64(0));
    Label arrayCreateByLength(env);
    Label argIsNumber(env);
    BRANCH(TaggedIsNumber(arg0), &argIsNumber, &slowPath);
    Bind(&argIsNumber);
    {
        Label argIsInt(env);
        Label argIsDouble(env);
        BRANCH(TaggedIsInt(arg0), &argIsInt, &argIsDouble);
        Bind(&argIsInt);
        {
            Label validIntLength(env);
            GateRef intLen = GetInt64OfTInt(arg0);
            GateRef isGEZero = Int64GreaterThanOrEqual(intLen, Int64(0));
            GateRef isLEMaxLen = Int64LessThanOrEqual(intLen, Int64(JSObject::MAX_GAP));
            BRANCH(BitAnd(isGEZero, isLEMaxLen), &validIntLength, &slowPath);
            Bind(&validIntLength);
            {
                arrayLength = intLen;
                Jump(&arrayCreateByLength);
            }
        }
        Bind(&argIsDouble);
        {
            Label validDoubleLength(env);
            GateRef doubleLength = GetDoubleOfTDouble(arg0);
            GateRef doubleToInt = DoubleToInt(glue, doubleLength, base::INT32_BITS);
            GateRef intToDouble = CastInt64ToFloat64(SExtInt32ToInt64(doubleToInt));
            GateRef doubleEqual = DoubleEqual(doubleLength, intToDouble);
            GateRef doubleLEMaxLen = DoubleLessThanOrEqual(doubleLength, Double(JSObject::MAX_GAP));
            BRANCH(BitAnd(doubleEqual, doubleLEMaxLen), &validDoubleLength, &slowPath);
            Bind(&validDoubleLength);
            {
                arrayLength = SExtInt32ToInt64(doubleToInt);
                Jump(&arrayCreateByLength);
            }
        }
    }
    Bind(&arrayCreateByLength);
    {
        NewObjectStubBuilder newBuilder(env, globalEnv);
        newBuilder.SetParameters(glue, 0);
        GateRef truncedLength = TruncInt64ToInt32(*arrayLength);
        res = newBuilder.NewFloat32ArrayWithSize(glue, truncedLength);
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        // no need to alloc in slowpath.
        GateRef thisObj = Undefined();
        GateRef argc = Int64(4); // 4: means func newtarget thisObj arg0
        GateRef argv = IntPtr(0);
        Label isEnableCMCGC(env);
        Label skipReadBarrier(env);
        BRANCH_UNLIKELY(LoadPrimitive(VariableType::BOOL(), glue,
                                      IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
                        &isEnableCMCGC, &skipReadBarrier);
        Bind(&isEnableCMCGC);
        {
            Label readBarrier(env);
            BRANCH_LIKELY(NeedSkipReadBarrier(glue), &skipReadBarrier, &readBarrier);
            Bind(&readBarrier);

            CallNGCRuntime(glue, RTSTUB_ID(CopyCallTarget), {glue, ctor});
            Jump(&skipReadBarrier);
        }
        Bind(&skipReadBarrier);
        std::vector<GateRef> args { glue, argc, argv, ctor, ctor, thisObj, arg0 };
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCallNew));
        GateRef target = IntPtr(RTSTUB_ID(JSCallNew));
        auto depend = env->GetCurrentLabel()->GetDepend();
        res = env->GetBuilder()->Call(cs, glue, target, depend, args,
                                      Circuit::NullGate(), "NewFloat32Array stub slowpath");
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

void StringLoadElementStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);          // 0: first argument glue
    GateRef string = TaggedArgument(1);     // 1: second argument string
    GateRef index = Int32Argument(2);       // 2: 3rd argument index
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument global env
    BuiltinsStringStubBuilder builder(this, globalEnv);
    GateRef result = builder.GetSingleCharCodeByIndex(glue, string, index);
    Return(result);
}

void GetStringFromConstPoolStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef constpool = TaggedArgument(1);
    GateRef index = Int32Argument(2); // index
    GateRef result = GetStringFromConstPool(glue, constpool, index);
    Return(result);
}

void GetPrototypeStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    auto &builder = *env->GetBuilder();
    GateRef glue = PtrArgument(0);
    GateRef func = TaggedArgument(1);
    Return(builder.GetPrototype(glue, func));
}

void FastCallSelectorStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    auto &builder = *env->GetBuilder();
    GateRef glue = PtrArgument(0);
    GateRef func = TaggedArgument(1);
    GateRef actualArgc = Int64Argument(2); /* 2 : 3rd parameter is actualArgc */

    Label entry(env);
    Label exit(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), builder.Int32(static_cast<int32_t>(FastCallType::SLOW_CALL)));
    CallCoStubBuilder::FastCallSelector(builder, glue, func, actualArgc, &result, &exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    Return(ret);
}

void CheckSuperAndNewStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef super = TaggedArgument(1);
    GateRef newTarget = TaggedArgument(2); /* 2 : 3rd parameter is newTarget */
    NewObjectStubBuilder objBuilder(env);
    Label isHeapObj(env);
    Label isJsFunc(env);
    Label isCtor(env);
    Label needAllocateThis(env);
    Label defaultThis(env);
    Label exit(env);

    BRANCH(TaggedIsHeapObject(super), &isHeapObj, &exit);
    Bind(&isHeapObj);
    BRANCH(IsJSFunction(glue, super), &isJsFunc, &exit);
    Bind(&isJsFunc);
    BRANCH(IsConstructor(glue, super), &isCtor, &exit);
    Bind(&isCtor);
    BRANCH(IsBase(glue, super), &needAllocateThis, &defaultThis);
    Bind(&needAllocateThis);
    Return(objBuilder.FastSuperAllocateThis(glue, super, newTarget));
    Bind(&defaultThis);
    Return(Undefined());
    Bind(&exit);
    Return(Hole());
}

void SuperCallAndConstructorCheckStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef super = TaggedArgument(1);
    GateRef newTarget = TaggedArgument(2); /* 2 : 3rd parameter is newtarget */
    GateRef thisObj = TaggedArgument(3);   /* 3 : 4th parameter is thisObj   */
    GateRef argc = Int64Argument(4);       /* 4 : 5th parameter is argc      */
    GateRef argv = PtrArgument(5);         /* 5 : 6th parameter is argv      */

    GateRef gate = Circuit::NullGate();
    auto &builder = *env->GetBuilder();
    NewObjectStubBuilder objBuilder(env);

    Label entry(env);
    Label exit(env);

    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    auto args = {gate, super, newTarget, thisObj, argc};
    CallCoStubBuilder::LowerFastSuperCall(glue, builder, args, argv, result, exit);
    Bind(&exit);
    result = objBuilder.ConstructorCheck(glue, super, *result, thisObj);
    auto ret = *result;
    env->SubCfgExit();
    Return(ret);
}

void ConvertCharToInt32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef charCode = Int32Argument(1);
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd argument global env
    BuiltinsStringStubBuilder builder(this, globalEnv);
    // char to string
    GateRef result = builder.CreateStringBySingleCharCode(glue, charCode);
    // string to number
    result = CallNGCRuntime(glue, RTSTUB_ID(StringToNumber), {glue, result, Int32(0)}, charCode);
    // get int from number
    result = NumberGetInt(glue, result);
    Return(result);
}

void ConvertCharToDoubleStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef charCode = Int32Argument(1);
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd argument global env
    BuiltinsStringStubBuilder builder(this, globalEnv);
    // char to string
    GateRef result = builder.CreateStringBySingleCharCode(glue, charCode);
    // string to number
    result = CallNGCRuntime(glue, RTSTUB_ID(StringToNumber), {glue, result, Int32(0)}, charCode);
    // get double from number
    result = GetDoubleOfTNumber(result);
    Return(result);
}

void ConvertCharToStringStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef charCode = Int32Argument(1);
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd argument global env
    BuiltinsStringStubBuilder builder(this, globalEnv);
    // char to string
    GateRef result = builder.CreateStringBySingleCharCode(glue, charCode);
    Return(result);
}

void MulStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Mul(glue, x, y));
}

void DivStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Div(glue, x, y));
}

void ModStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
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
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Equal(glue, x, y));
}

void NotEqualStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.NotEqual(glue, x, y));
}

void StrictEqualStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.StrictEqual(glue, x, y));
}

void StrictNotEqualStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.StrictNotEqual(glue, x, y));
}

void LessStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Less(glue, x, y));
}

void LessEqStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.LessEq(glue, x, y));
}

void GreaterStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Greater(glue, x, y));
}

void GreaterEqStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.GreaterEq(glue, x, y));
}

void ShlStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Shl(glue, x, y));
}

void ShrStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Shr(glue, x, y));
}

void AshrStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Ashr(glue, x, y));
}

void AndStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.And(glue, x, y));
}

void OrStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Or(glue, x, y));
}

void XorStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    OperationsStubBuilder operationBuilder(this, globalEnv);
    Return(operationBuilder.Xor(glue, x, y));
}

void IsInStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef prop = TaggedArgument(1); // 1: 2nd argument
    GateRef obj = TaggedArgument(2);  // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    SetCurrentGlobalEnv(globalEnv);
    Return(IsIn(glue, prop, obj));
}

void InstanceofStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef object = TaggedArgument(1);
    GateRef target = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    GateRef jsFunc = TaggedArgument(4);     // 4 : 5th para
    GateRef slotId = Int32Argument(5);      // 5 : 6th pars
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    SetCurrentGlobalEnv(globalEnv);
    Return(InstanceOf(glue, object, target, profileTypeInfo, slotId, ProfileOperation()));
}

void OrdinaryHasInstanceStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef object = TaggedArgument(1);
    GateRef target = TaggedArgument(2); // 2: 3rd argument
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th argument
    SetCurrentGlobalEnv(globalEnv);
    Return(OrdinaryHasInstance(glue, target, object));
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
    Return(FastToBoolean(glue, x, true));
}

void ToBooleanFalseStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    Return(FastToBoolean(glue, x, false));
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
    GateRef globalEnv = GetCurrentGlobalEnv(glue, env);
    NewObjectStubBuilder newBuilder(this, globalEnv);
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
    GateRef globalEnv = TaggedArgument(5);       /* 5 : 6th parameter is globalEnv */

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
    NewObjectStubBuilder newBuilder(this, globalEnv);
    newBuilder.SetParameters(glue, 0);
    GateRef intialHClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                             static_cast<size_t>(GlobalEnvField::ELEMENT_HOLE_TAGGED_HCLASS_INDEX));
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
    GateRef globalEnv = TaggedArgument(4);       /* 4 : 5th parameter is globalEnv */

    DEFVARIABLE(argumentsList, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(argumentsObj, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(argv, VariableType::NATIVE_POINTER(), PtrArgument(1));
    Label afterArgumentsList(env);
    Label newArgumentsObj(env);
    Label exit(env);

    GateRef actualArgc = Int32Sub(numArgs, Int32(NUM_MANDATORY_JSFUNC_ARGS));
    GateRef startIdx = Int32(0);
    NewObjectStubBuilder newBuilder(this, globalEnv);
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
    GateRef globalEnv = TaggedArgument(2);  // 2 : 3rd parameter is globalEnv
    SetCurrentGlobalEnv(globalEnv);
    Return(GetCallSpreadArgs(glue, array, ProfileOperation()));
}

void GetPropertyByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th parameter is globalEnv
    SetCurrentGlobalEnv(globalEnv);
    Return(GetPropertyByIndex(glue, receiver, index, ProfileOperation()));
}

void SetPropertyByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th parameter is globalEnv
    SetCurrentGlobalEnv(globalEnv);
    Return(SetPropertyByIndex(glue, receiver, index, value, false));
}

void SetPropertyByIndexWithOwnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th parameter is globalEnv
    SetCurrentGlobalEnv(globalEnv);
    Return(SetPropertyByIndex(glue, receiver, index, value, true));
}

void JSTaggedValueHasPropertyStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2);      // 2 : 3rd para
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th parameter is globalEnv
    SetCurrentGlobalEnv(globalEnv);
    Return(HasProperty(glue, obj, key));
}

void GetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef id = Int64Argument(2); // 2 : 3rd para
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para
    GateRef jsFunc = TaggedArgument(4);     // 4 : 5th para
    GateRef slotId = Int32Argument(5);      // 5 : 6th para
    AccessObjectStubBuilder builder(this, globalEnv, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.LoadObjByName(glue, receiver, id, info, profileTypeInfo, slotId, ProfileOperation()));
}

void DeprecatedGetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedPointerArgument(2); // 2 : 3rd para
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para
    AccessObjectStubBuilder builder(this, globalEnv);
    Return(builder.DeprecatedLoadObjByName(glue, receiver, key));
}

void SetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef id = Int64Argument(2); // 2 : 3rd para
    GateRef value = TaggedPointerArgument(3); // 3 : 4th para
    GateRef globalEnv = TaggedArgument(4);    // 4: 5th para
    GateRef jsFunc = TaggedArgument(5);       // 5 : 6th para
    GateRef slotId = Int32Argument(6);        // 6 : 7th para
    AccessObjectStubBuilder builder(this, globalEnv, jsFunc);
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
    GateRef globalEnv = TaggedArgument(5);  // 5: 6th para
    GateRef jsFunc = TaggedArgument(6);     // 6 : 7th para
    GateRef slotId = Int32Argument(7);      // 7 : 8th para
    AccessObjectStubBuilder builder(this, globalEnv, jsFunc);
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
    GateRef globalEnv = TaggedArgument(6);  // 6: 7th para
    GateRef jsFunc = TaggedArgument(7);     // 7 : 8th para
    GateRef slotId = Int32Argument(8);      // 8 : 9th para
    AccessObjectStubBuilder builder(this, globalEnv, jsFunc);
    Return(builder.StoreObjByNameWithMega(glue, receiver, value, megaStubCache, prop, jsFunc, slotId,
                                          ProfileOperation()));
}

void DeprecatedSetPropertyByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th para
    SetCurrentGlobalEnv(globalEnv);
    Return(SetPropertyByName(glue, receiver, key, value, false, True()));
}

void SetPropertyByNameWithOwnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th para
    SetCurrentGlobalEnv(globalEnv);
    Return(SetPropertyByName(glue, receiver, key, value, true, True()));
}

void GetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para
    GateRef jsFunc = TaggedArgument(4);     // 4 : 5th para
    GateRef slotId = Int32Argument(5);      // 5 : 6th para
    AccessObjectStubBuilder builder(this, globalEnv);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.LoadObjByValue(glue, receiver, key, profileTypeInfo, slotId));
}

void DeprecatedGetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para
    SetCurrentGlobalEnv(globalEnv);
    Return(GetPropertyByValue(glue, receiver, key));
}

void SetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);        // 2 : 3rd para
    GateRef value = TaggedArgument(3);      // 3 : 4th para
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th para
    GateRef jsFunc = TaggedArgument(5);     // 5 : 6th para
    GateRef slotId = Int32Argument(6);      // 6 : 7th para
    AccessObjectStubBuilder builder(this, globalEnv);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.StoreObjByValue(glue, receiver, key, value, profileTypeInfo, slotId));
}

void DeprecatedSetPropertyByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    GateRef globalEnv = TaggedArgument(4);        // 4: 5th parameter is globalEnv
    SetCurrentGlobalEnv(globalEnv);
    Return(SetPropertyByValue(glue, receiver, key, value, false));
}

void SetPropertyByValueWithOwnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    GateRef globalEnv = TaggedArgument(4);        // 4: 5th parameter is globalEnv
    SetCurrentGlobalEnv(globalEnv);
    Return(SetPropertyByValue(glue, receiver, key, value, true));
}

void StOwnByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th parameter is globalEnv
    AccessObjectStubBuilder builder(this, globalEnv);
    Return(builder.StOwnByIndex(glue, receiver, index, value));
}

void StOwnByValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    GateRef globalEnv = TaggedArgument(4);        // 4: 5th parameter is globalEnv
    AccessObjectStubBuilder builder(this, globalEnv);
    Return(builder.StOwnByValue(glue, receiver, key, value));
}

void StOwnByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th para
    AccessObjectStubBuilder builder(this, globalEnv);
    Return(builder.StOwnByName(glue, receiver, key, value));
}

void StOwnByValueWithNameSetStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    GateRef globalEnv = TaggedArgument(4);        // 4: 5th parameter is globalEnv
    AccessObjectStubBuilder builder(this, globalEnv);
    Return(builder.StOwnByValueWithNameSet(glue, receiver, key, value));
}

void StOwnByNameWithNameSetStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th para
    AccessObjectStubBuilder builder(this, globalEnv);
    Return(builder.StOwnByNameWithNameSet(glue, receiver, key, value));
}

void LdObjByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th parameter is globalEnv
    AccessObjectStubBuilder builder(this, globalEnv);
    Return(builder.LdObjByIndex(glue, receiver, index));
}

void StObjByIndexStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th parameter is globalEnv
    AccessObjectStubBuilder builder(this, globalEnv);
    Return(builder.StObjByIndex(glue, receiver, index, value));
}

void TryLdGlobalByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef globalEnv = TaggedArgument(2); // 2 : 3rd para
    GateRef jsFunc = TaggedArgument(3); // 3 : 4th para
    GateRef slotId = Int32Argument(4); // 4 : 5th para
    AccessObjectStubBuilder builder(this, globalEnv, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.TryLoadGlobalByName(glue, id, info, profileTypeInfo, slotId, ProfileOperation()));
}

void TryStGlobalByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef value = TaggedArgument(2); // 2 : 3rd para
    GateRef globalEnv = TaggedArgument(3); // 3 : 4th para
    GateRef jsFunc = TaggedArgument(4); // 4 : 5th para
    GateRef slotId = Int32Argument(5);  // 5: 6th para
    AccessObjectStubBuilder builder(this, globalEnv, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.TryStoreGlobalByName(glue, id, info, value, profileTypeInfo, slotId, ProfileOperation()));
}

void LdGlobalVarStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef globalEnv = TaggedArgument(2);  // 2 : 3rd para
    GateRef jsFunc = TaggedArgument(3);     // 3 : 4th para
    GateRef slotId = Int32Argument(4);      // 4 : 5th para
    AccessObjectStubBuilder builder(this, globalEnv, jsFunc);
    StringIdInfo info(0, 0, StringIdInfo::Offset::INVALID, StringIdInfo::Length::INVALID);
    GateRef profileTypeInfo = UpdateProfileTypeInfo(glue, jsFunc);
    Return(builder.LoadGlobalVar(glue, id, info, profileTypeInfo, slotId, ProfileOperation()));
}

void StGlobalVarStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef id = Int64Argument(1);
    GateRef value = TaggedArgument(2); // 2 : 3rd para
    GateRef globalEnv = TaggedArgument(3);  // 3 : 4th para
    GateRef jsFunc = TaggedArgument(4);     // 4 : 5th para
    GateRef slotId = Int32Argument(5);      // 5: 6th para
    AccessObjectStubBuilder builder(this, globalEnv, jsFunc);
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
        GateRef hclass = LoadHClass(glue, receiver);
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        {
            Return(LoadICWithHandler(glue, receiver, receiver, secondValue, ProfileOperation()));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(glue, firstValue, hclass);
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
    GateRef globalEnv = TaggedArgument(5);   // 5: 6th parameter is globalEnv

    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label firstValueEqualKey(env);
    Label cachedHandlerNotHole(env);
    BRANCH(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(glue, receiver);
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        SetCurrentGlobalEnv(globalEnv);
        Return(LoadElement(glue, receiver, key));
        Bind(&hclassNotEqualFirstValue);
        {
            BRANCH(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                auto cachedHandler = CheckPolyHClass(glue, secondValue, hclass);
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
        GateRef hclass = LoadHClass(glue, receiver);
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        {
            Return(StoreICWithHandler(glue, receiver, receiver, value, secondValue));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(glue, firstValue, hclass);
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
    GateRef globalEnv = TaggedArgument(6);  // 6: 7th parameter is globalEnv
    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label firstValueEqualKey(env);
    Label cachedHandlerNotHole(env);
    BRANCH(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(glue, receiver);
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
               &hclassEqualFirstValue,
               &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        SetCurrentGlobalEnv(globalEnv);
        Return(ICStoreElement(glue, receiver, key, value, secondValue));
        Bind(&hclassNotEqualFirstValue);
        {
            BRANCH(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                GateRef cachedHandler = CheckPolyHClass(glue, secondValue, hclass);
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

void CMCSetValueWithBarrierStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef offset = PtrArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    CMCSetValueWithBarrier(glue, obj, offset, value);
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

void GetValueWithBarrierStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef addr = TaggedArgument(1);
    GateRef value = GetValueWithBarrier(glue, addr);
    Return(value);
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
    GateRef globalEnv = TaggedArgument(1);  // 1 : 2nd para
    NewObjectStubBuilder newBuilder(this, globalEnv);
    Return(newBuilder.CreateEmptyArray(glue));
}

void CreateArrayWithBufferStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef index = Int32Argument(1);
    GateRef jsFunc = TaggedArgument(2); // 2 : 3rd para
    GateRef slotId = Int32Argument(3);  // 3 : 4th para
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th para
    NewObjectStubBuilder newBuilder(this, globalEnv);
    Return(newBuilder.CreateArrayWithBuffer(
        glue, index, jsFunc, { IntPtr(0), 0, true }, Undefined(), slotId, ProfileOperation()));
}

void NewJSObjectStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef hclass = TaggedArgument(1);
    GateRef size = Int64Argument(2); // size
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.NewJSObject(glue, hclass, size));
}

void FastNewThisObjectStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef ctor = TaggedArgument(1);
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.FastNewThisObject(glue, ctor));
}

void FastSuperAllocateThisStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef superCtor = TaggedArgument(1);
    GateRef newtarget = TaggedArgument(2);
    NewObjectStubBuilder newBuilder(this);
    Return(newBuilder.FastSuperAllocateThis(glue, superCtor, newtarget));
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
    GateRef method = GetMethodFromFunction(glue, func);
    GateRef callfield = LoadPrimitive(VariableType::INT64(), method, IntPtr(Method::CALL_FIELD_OFFSET));
    GateRef expectedNum = Int64And(Int64LSR(callfield, Int64(MethodLiteral::NumArgsBits::START_BIT)),
        Int64((1LU << MethodLiteral::NumArgsBits::SIZE) - 1));
    GateRef expectedArgc = Int64Add(expectedNum, Int64(NUM_MANDATORY_JSFUNC_ARGS));
    GateRef actualArgc = Int64Sub(argc, IntPtr(NUM_MANDATORY_JSFUNC_ARGS));

    Label isEnableCMCGC(env);
    Label skipReadBarrier(env);
    BRANCH_UNLIKELY(
        LoadPrimitive(VariableType::BOOL(), glue, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &isEnableCMCGC, &skipReadBarrier);
    Bind(&isEnableCMCGC);
    {
        Label readBarrier(env);
        BRANCH_LIKELY(NeedSkipReadBarrier(glue), &skipReadBarrier, &readBarrier);
        Bind(&readBarrier);
        CallNGCRuntime(glue, RTSTUB_ID(CopyCallTarget), {glue, func});
        CallNGCRuntime(glue, RTSTUB_ID(CopyArgvArray), {glue, argv, argc});
        Jump(&skipReadBarrier);
    }

    Bind(&skipReadBarrier);
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
    GateRef glue = PtrArgument(0);
    GateRef str = TaggedArgument(1);
    GateRef index = Int32Argument(2);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para
    BuiltinsStringStubBuilder builder(this, globalEnv);
    GateRef result = builder.GetSingleCharCodeByIndex(glue, str, index);
    Return(result);
}

void CreateStringBySingleCharCodeStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef charCode = Int32Argument(1);
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd para
    BuiltinsStringStubBuilder builder(this, globalEnv);
    GateRef result = builder.CreateStringBySingleCharCode(glue, charCode);
    Return(result);
}

void FastStringEqualStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef str1 = TaggedArgument(1);
    GateRef str2 = Int32Argument(2);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para

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
        SetCurrentGlobalEnv(globalEnv);
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
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para

    BuiltinsStringStubBuilder builtinsStringStubBuilder(this, globalEnv);
    GateRef result = builtinsStringStubBuilder.StringConcat(glue, str1, str2);
    Return(result);
}

void StringAddStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef str1 = TaggedArgument(1);
    GateRef str2 = TaggedArgument(2);       // 2: 3rd argument
    GateRef status = Int32Argument(3);      // 3: 4th argument
    GateRef globalEnv = TaggedArgument(4);  // 4: 5th argument

    BuiltinsStringStubBuilder builtinsStringStubBuilder(this, globalEnv);
    GateRef result = builtinsStringStubBuilder.StringAdd(glue, str1, str2, status);
    Return(result);
}

void DeleteObjectPropertyStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef object = TaggedArgument(1);
    GateRef prop = TaggedArgument(2);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para
    SetCurrentGlobalEnv(globalEnv);
    GateRef result = DeletePropertyOrThrow(glue, object, prop);
    Return(result);
}

void GetpropiteratorStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef object = TaggedArgument(1);
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd para
    NewObjectStubBuilder newBuilder(this, globalEnv);
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
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd para

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    BuiltinsStringStubBuilder builder(this, globalEnv);
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
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd para

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    BuiltinsArrayStubBuilder builder(this, globalEnv);
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
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd para

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    BuiltinsCollectionIteratorStubBuilder<JSMapIterator> builder(this, glue, obj, Gate::InvalidGateRef, globalEnv);
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
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd para

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    BuiltinsCollectionIteratorStubBuilder<JSSetIterator> builder(this, glue, obj, Gate::InvalidGateRef, globalEnv);
    builder.Next(&result, &exit);
    Bind(&exit);
    Return(*result);
}

void GetIteratorStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef globalEnv = TaggedArgument(2);  // 2: 3rd para

    SetCurrentGlobalEnv(globalEnv);
    GateRef res = GetIterator(glue, obj, ProfileOperation());
    Return(res);
}

void JSMapGetStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para

    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> builder(this, glue, globalEnv);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Get(linkedTable, key));
}

void JSMapHasStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para

    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> builder(this, glue, globalEnv);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Has(linkedTable, key));
}

void JSSetHasStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para

    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> builder(this, glue, globalEnv);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Has(linkedTable, key));
}

void JSProxyGetPropertyStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef holder = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);
    GateRef receiver = TaggedArgument(3U);
    GateRef globalEnv = TaggedArgument(4U);
    BuiltinsProxyStubBuilder proxyStubBuilder(this, glue, globalEnv);
    GateRef result = proxyStubBuilder.GetProperty(holder, key, receiver);
    Return(result);
}

void JSProxySetPropertyStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef holder = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);
    GateRef value = TaggedArgument(3U);
    GateRef receiver = TaggedArgument(4U);
    GateRef globalEnv = TaggedArgument(5U);
    BuiltinsProxyStubBuilder proxyStubBuilder(this, glue, globalEnv);
    GateRef result = proxyStubBuilder.SetProperty(holder, key, value, receiver, true);
    Return(result);
}

void JSProxySetPropertyNoThrowStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef holder = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);
    GateRef value = TaggedArgument(3U);
    GateRef receiver = TaggedArgument(4U);
    GateRef globalEnv = TaggedArgument(5U);
    BuiltinsProxyStubBuilder proxyStubBuilder(this, glue, globalEnv);
    GateRef result = proxyStubBuilder.SetProperty(holder, key, value, receiver, false);
    Return(result);
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
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para

    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> builder(this, glue, globalEnv);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Delete(linkedTable, key));
}

void JSSetDeleteStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para

    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> builder(this, glue, globalEnv);
    GateRef linkedTable = builder.GetLinked(obj);
    Return(builder.Delete(linkedTable, key));
}

void JSSetAddStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef key = TaggedArgument(2U);
    GateRef globalEnv = TaggedArgument(3);  // 3: 4th para

    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> builder(this, glue, globalEnv);
    GateRef linkedTable = builder.GetLinked(obj);
    GateRef newTable = builder.Insert(linkedTable, key, key);
    builder.Store(VariableType::JS_ANY(), glue, obj, IntPtr(JSSet::LINKED_SET_OFFSET), newTable);
    Return(obj);
}

void GrowElementsCapacityStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef thisValue = TaggedArgument(1);
    GateRef globalEnv = TaggedArgument(2U);
    GateRef newLength = Int32Argument(3U);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    BuiltinsArrayStubBuilder builder(this, globalEnv);
    result = builder.GrowElementsCapacity(glue, thisValue, newLength);
    Return(*result);
}

void SameValueStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef left = TaggedArgument(1);
    GateRef right = TaggedArgument(2U);
    GateRef globalEnv = TaggedArgument(3U); // 3 : 4th para
    SetCurrentGlobalEnv(globalEnv);
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

void ReverseBarrierStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef dstObj = PtrArgument(1);
    GateRef dstAddr = PtrArgument(2);
    GateRef taggedValueCount = TaggedArgument(3);
    BarrierStubBuilder barrierBuilder(this, glue, dstObj, dstAddr, taggedValueCount);
    barrierBuilder.DoReverseBarrier();
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
    GateRef entry = FindEntryFromHashTable<NameDictionary>(glue, taggedArray, key);
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

#define INIT_SIGNATURES_DYN(name, base)                                                    \
    base##CallSignature::Initialize(&callSigns_[name]);                                    \
    callSigns_[name].SetID(name);                                                          \
    callSigns_[name].SetName(std::string("COStub_") + #name);                              \
    callSigns_[name].SetConstructor(                                                       \
        [](void* env) {                                                                    \
            return static_cast<void*>(                                                     \
                new name##StubBuilder(&callSigns_[name], static_cast<Environment*>(env))); \
        });                                                                                \
    callSigns_[name].SetStwCopyStub(true);                                                 \
    callSigns_[name].SetTargetKind(CallSignature::TargetKind::COMMON_STW_COPY_STUB);

#define INIT_SIGNATURES_DYN_SECOND(base)                                                   \
    INIT_SIGNATURES_DYN(base##StwCopy, base)

    COMMON_STW_COPY_STUB_LIST(INIT_SIGNATURES_DYN_SECOND)
#undef INIT_SIGNATURES_DYN_SECOND
#undef INIT_SIGNATURES_DYN
}

void CommonStubCSigns::GetCSigns(std::vector<const CallSignature*>& outCSigns)
{
    for (size_t i = 0; i < NUM_OF_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}
}  // namespace panda::ecmascript::kungfu
