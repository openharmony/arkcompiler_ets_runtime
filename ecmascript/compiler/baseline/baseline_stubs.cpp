/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/baseline/baseline_stubs.h"
#include "ecmascript/compiler/baseline/baseline_call_signature.h"
#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/profiler_stub_builder.h"
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/compiler/baseline/baseline_stub_builder.h"
#include "ecmascript/js_async_generator_object.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

#define PARAM_INDEX(opcode, param) static_cast<size_t>(opcode##CallSignature::ParameterIndex::param)

#define METHOD_ENTRY_ENV_DEFINED(func)                                                            \
    GateRef isDebugModeOffset = IntPtr(JSThread::GlueData::GetIsDebugModeOffset(env->Is32Bit())); \
    GateRef isDebugMode = Load(VariableType::BOOL(), glue, isDebugModeOffset);                    \
    Label isDebugModeTrue(env);                                                                   \
    Label isDebugModeFalse(env);                                                                  \
    BRANCH(isDebugMode, &isDebugModeTrue, &isDebugModeFalse);                                     \
    Bind(&isDebugModeTrue);                                                                       \
    {                                                                                             \
        CallRuntime(glue, RTSTUB_ID(MethodEntry), { func });                                      \
        Jump(&isDebugModeFalse);                                                                  \
    }                                                                                             \
    Bind(&isDebugModeFalse)

#define UPDATE_HOTNESS(func, callback)                                                                         \
    varHotnessCounter = Int32Add(offset, *varHotnessCounter);                                                  \
    BRANCH(Int32LessThan(*varHotnessCounter, Int32(0)), &slowPath, &dispatch);                                 \
    Bind(&slowPath);                                                                                           \
    {                                                                                                          \
        GateRef iVecOffset = IntPtr(JSThread::GlueData::GetInterruptVectorOffset(env->IsArch32Bit()));         \
        GateRef interruptsFlag = Load(VariableType::INT8(), glue, iVecOffset);                                 \
        varHotnessCounter = Int32(EcmaInterpreter::METHOD_HOTNESS_THRESHOLD);                                  \
        Label initialized(env);                                                                                \
        Label callRuntime(env);                                                                                \
        BRANCH(BoolOr(TaggedIsUndefined(*varProfileTypeInfo),                                                  \
                      Int8Equal(interruptsFlag, Int8(VmThreadControl::VM_NEED_SUSPENSION))),                   \
            &callRuntime, &initialized);                                                                       \
        Bind(&callRuntime);                                                                                    \
        if (!(callback).IsEmpty()) {                                                                           \
            varProfileTypeInfo = CallRuntime(glue, RTSTUB_ID(UpdateHotnessCounterWithProf), { func });         \
        } else {                                                                                               \
            varProfileTypeInfo = CallRuntime(glue, RTSTUB_ID(UpdateHotnessCounter), { func });                 \
        }                                                                                                      \
        Label handleException(env);                                                                            \
        Label noException(env);                                                                                \
        BRANCH(HasPendingException(glue), &handleException, &noException);                                     \
        Bind(&handleException);                                                                                \
        {                                                                                                      \
            Jump(&dispatch);                                                                                   \
        }                                                                                                      \
        Bind(&noException);                                                                                    \
        {                                                                                                      \
            Jump(&dispatch);                                                                                   \
        }                                                                                                      \
        Bind(&initialized);                                                                                    \
        (callback).TryDump();                                                                                  \
        (callback).TryJitCompile();                                                                            \
        Jump(&dispatch);                                                                                       \
    }                                                                                                          \
    Bind(&dispatch);

enum ActualNumArgsOfCall : uint8_t { CALLARG0 = 0, CALLARG1, CALLARGS2, CALLARGS3 };

CallSignature BaselineStubCSigns::callSigns_[BaselineStubCSigns::NUM_OF_STUBS];


#define CHECK_EXCEPTION(res, offset)                                                      \
    CheckException(acc, res)

#define CHECK_EXCEPTION_VARACC(res, offset)                                               \
    CheckException(*varAcc, res)

#define CHECK_EXCEPTION_WITH_JUMP(res, jump)                                              \
    CheckExceptionWithJump(acc, res, jump)

#define CHECK_EXCEPTION_WITH_ACC(res, offset)                                             \
    CheckExceptionWithVar(acc, res)

#define CHECK_EXCEPTION_WITH_VARACC(res, offset)                                          \
    CheckExceptionWithVar(*varAcc, res)

#define CHECK_PENDING_EXCEPTION(res, offset)                                              \
    CheckPendingException(glue, res, offset)

#define INT_PTR(opcode)                                                                   \
    IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Opcode::opcode))

#define METHOD_ENTRY(func)                                                                        \
    auto env = GetEnvironment();                                                                  \
    METHOD_ENTRY_ENV_DEFINED(func)

#define METHOD_EXIT()                                                                             \
    GateRef isDebugModeOffset = IntPtr(JSThread::GlueData::GetIsDebugModeOffset(env->Is32Bit())); \
    GateRef isDebugMode = Load(VariableType::BOOL(), glue, isDebugModeOffset);                    \
    GateRef isTracingOffset = IntPtr(JSThread::GlueData::GetIsTracingOffset(env->Is32Bit()));     \
    GateRef isTracing = Load(VariableType::BOOL(), glue, isTracingOffset);                        \
    Label NeedCallRuntimeTrue(env);                                                               \
    Label NeedCallRuntimeFalse(env);                                                              \
    Branch(BoolOr(isDebugMode, isTracing), &NeedCallRuntimeTrue, &NeedCallRuntimeFalse);          \
    Bind(&NeedCallRuntimeTrue);                                                                   \
    {                                                                                             \
        CallRuntime(glue, RTSTUB_ID(MethodExit), {});                                             \
        Jump(&NeedCallRuntimeFalse);                                                              \
    }                                                                                             \
    Bind(&NeedCallRuntimeFalse)

#define DISPATCH(opcode) DISPATCH_BAK(OFFSET, INT_PTR(opcode))
#define DISPATCH_BAK(TYPE, ...) DISPATCH_##TYPE(__VA_ARGS__)
#define DISPATCH_OFFSET(offset)                                                           \
    DISPATCH_BASE(profileTypeInfo, acc, hotnessCounter, offset)
#define DISPATCH_BASE(...)                                                                \
    Dispatch(glue, sp, pc, constpool, __VA_ARGS__)

#define DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineBianryOP)                              \
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineBianryOP, GLUE));                              \
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineBianryOP, SP));                                  \
    GateRef left = TaggedArgument(PARAM_INDEX(BaselineBianryOP, LEFT));                           \
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineBianryOP, SLOT_ID));                       \
                                                                                                  \
    GateRef frame = GetFrame(sp);                                                                 \
    GateRef acc = GetAccFromFrame(frame);                                                         \
    GateRef func = GetFunctionFromFrame(frame);                                                   \
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);                               \
                                                                                                  \
    ProfileOperation callback(                                                                    \
        [this, glue, func, slotId, profileTypeInfo](const std::initializer_list<GateRef> &values, \
                                                    OperationType type) {                         \
            ProfilerStubBuilder profiler(this);                                                   \
            profiler.PGOProfiler(glue, func, profileTypeInfo, slotId, values, type);              \
        }, nullptr)

#define DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineBianryOP)                              \
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineBianryOP, GLUE));                              \
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineBianryOP, SP));                                  \
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineBianryOP, SLOT_ID));                       \
                                                                                                  \
    GateRef frame = GetFrame(sp);                                                                 \
    GateRef acc = GetAccFromFrame(frame);                                                         \
    GateRef func = GetFunctionFromFrame(frame);                                                   \
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);                               \
                                                                                                  \
    ProfileOperation callback(                                                                    \
        [this, glue, func, slotId, profileTypeInfo](const std::initializer_list<GateRef> &values, \
                                                    OperationType type) {                         \
            ProfilerStubBuilder profiler(this);                                                   \
            profiler.PGOProfiler(glue, func, profileTypeInfo, slotId, values, type);              \
        }, nullptr)

void BaselineStubCSigns::Initialize()
{
#define INIT_SIGNATURES(name)                                                              \
    name##CallSignature::Initialize(&callSigns_[name]);                                    \
    callSigns_[name].SetID(name);                                                          \
    callSigns_[name].SetName(std::string("BaselineStub_") + #name);                        \
    callSigns_[name].SetConstructor(                                                       \
        [](void* env) {                                                                    \
            return static_cast<void*>(                                                     \
                new name##StubBuilder(&callSigns_[name], static_cast<Environment*>(env))); \
        });

    BASELINE_STUB_ID_LIST(INIT_SIGNATURES)
#undef INIT_SIGNATURES
}

void BaselineStubCSigns::GetCSigns(std::vector<const CallSignature*>& outCSigns)
{
    for (size_t i = 0; i < NUM_OF_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}

void BaselineLdObjByNameStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdObjByName, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdObjByName, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdObjByName, ACC));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineLdObjByName, PROFILE_TYPE_INFO));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdObjByName, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdObjByName, SLOT_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef receiver = acc;
    AccessObjectStubBuilder builder(this);
    ProfileOperation callback;
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.LoadObjByName(
        glue, receiver, 0, stringIdInfo, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineTryLdGLobalByNameImm8ID16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, ACC));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, PROFILE_TYPE_INFO));
    GateRef stringId = ZExtInt16ToInt32(Int32Argument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, STRING_ID)));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, SLOT_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    AccessObjectStubBuilder builder(this);
    ProfileOperation callback;
    StringIdInfo info(constpool, stringId);
    GateRef result = builder.TryLoadGlobalByName(glue, 0, info, profileTypeInfo, slotId, callback);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    Return(result);
}

void BaselineCallArg1Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallArg1Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallArg1Imm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallArg1Imm8V8, ACC));
    GateRef a0 = Int32Argument(PARAM_INDEX(BaselineCallArg1Imm8V8, ARG0));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallArg1Imm8V8, HOTNESS_COUNTER));
    ProfileOperation callback;

    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef actualNumArgs = Int32(ActualNumArgsOfCall::CALLARG1);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLARG1_IMM8_V8);
    GateRef result = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                               JSCallMode::CALL_ARG1, { a0Value }, callback);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = result;
    Return(result);
}

void BaselineStToGlobalRecordImm16ID16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStToGlobalRecordImm16ID16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStToGlobalRecordImm16ID16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStToGlobalRecordImm16ID16, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStToGlobalRecordImm16ID16, STRING_ID));
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedFalse() });
    Return(result);
}

void BaselineLdaStrID16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdaStrID16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdaStrID16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdaStrID16, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdaStrID16, STRING_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef result = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = result;
    Return(result);
}

void BaselineJmpImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef func = TaggedPointerArgument(SECOND_PARAMETER);
    GateRef profileTypeInfo = TaggedPointerArgument(THIRD_PARAMETER);
    GateRef hotnessCounter = Int32Argument(FOURTH_PARAMETER);
    GateRef offset = Int32Argument(FIFTH_PARAMETER);

    ProfileOperation callback;

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label dispatch(env);
    Label slowPath(env);

    UPDATE_HOTNESS(func, callback);
    Return();
}

void BaselineLdsymbolStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsymbol, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdsymbol, ACC));

    GateRef result = CallRuntime(glue, RTSTUB_ID(GetSymbolFunction), {});

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = result;
    Return(result);
}

void BaselineLdglobalStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdglobal, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdglobal, ACC));

    GateRef result = GetGlobalObject(glue);

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = result;
    Return(result);
}

void BaselinePoplexenvStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselinePoplexenv, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselinePoplexenv, SP));

    GateRef state = GetFrame(sp);
    GateRef currentLexEnv = GetEnvFromFrame(state);
    GateRef parentLexEnv = GetParentEnv(currentLexEnv);
    SetEnvToFrame(glue, state, parentLexEnv);
    Return();
}

void BaselineGetunmappedargsStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetunmappedargs, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetunmappedargs, ACC));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGetunmappedargs, SP));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(argumentsList, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(argumentsObj, VariableType::JS_ANY(), Hole());
    auto env = GetEnvironment();
    GateRef startIdxAndNumArgs = GetStartIdxAndNumArgs(sp, Int32(0));
    // 32: high 32 bits = startIdx, low 32 bits = numArgs
    GateRef startIdx = TruncInt64ToInt32(Int64LSR(startIdxAndNumArgs, Int64(32)));
    GateRef numArgs = TruncInt64ToInt32(startIdxAndNumArgs);

    Label newArgumentsObj(env);
    Label checkException(env);
    Label dispatch(env);
    Label slowPath(env);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterArgumentsList(env);
    newBuilder.NewArgumentsList(&argumentsList, &afterArgumentsList, sp, startIdx, numArgs);
    Bind(&afterArgumentsList);
    Branch(TaggedIsException(*argumentsList), &slowPath, &newArgumentsObj);
    Bind(&newArgumentsObj);
    Label afterArgumentsObj(env);
    newBuilder.NewArgumentsObj(&argumentsObj, &afterArgumentsObj, *argumentsList, numArgs);
    Bind(&afterArgumentsObj);
    Branch(TaggedIsException(*argumentsObj), &slowPath, &checkException);
    Bind(&checkException);
    Branch(HasPendingException(glue), &slowPath, &dispatch);
    Bind(&dispatch);
    {
        varAcc = *argumentsObj;
        Return(*argumentsObj);
    }

    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(GetUnmapedArgs), {});
        Return(res);
    }
}

void BaselineAsyncfunctionenterStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionenter, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionenter, ACC));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionEnter), {});

    Return(res);
}

void BaselineCreateasyncgeneratorobjV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateasyncgeneratorobjV8, GLUE));
    GateRef genFunc = TaggedArgument(PARAM_INDEX(BaselineCreateasyncgeneratorobjV8, GEN_FUNC));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionenter, ACC));

    (void) acc;
    auto env = GetEnvironment();
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateAsyncGeneratorObj), { genFunc });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        Return(result);
    }
    Bind(&notException);
    Return(result);
}

void BaselineDebuggerStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDebugger, GLUE));

    CallRuntime(glue, RTSTUB_ID(NotifyDebuggerStatement), {});
    Return();
}

void BaselineGetpropiteratorStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetpropiterator, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetpropiterator, ACC));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.EnumerateObjectProperties(glue, *varAcc);
    Return(res);
}

void BaselineGetiteratorImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetiteratorImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetiteratorImm8, ACC));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    ProfileOperation callback;
    GateRef res = GetIterator(glue, *varAcc, callback);

    Return(res);
}

void BaselineGetiteratorImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetiteratorImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetiteratorImm16, ACC));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    ProfileOperation callback;
    GateRef res = GetIterator(glue, *varAcc, callback);

    Return(res);
}

void BaselineCloseiteratorImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCloseiteratorImm8V8, GLUE));
    GateRef iter = TaggedArgument(PARAM_INDEX(BaselineCloseiteratorImm8V8, ITER));

    GateRef result = CallRuntime(glue, RTSTUB_ID(CloseIterator), { iter });
    Return(result);
}

void BaselineCloseiteratorImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCloseiteratorImm16V8, GLUE));
    GateRef iter = TaggedArgument(PARAM_INDEX(BaselineCloseiteratorImm16V8, ITER));

    GateRef result = CallRuntime(glue, RTSTUB_ID(CloseIterator), { iter });
    Return(result);
}

void BaselineAsyncgeneratorresolveV8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncgeneratorresolveV8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineAsyncgeneratorresolveV8V8V8, SP));
    GateRef offset = Int32Argument(PARAM_INDEX(BaselineAsyncgeneratorresolveV8V8V8, OFFSET));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineAsyncgeneratorresolveV8V8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineAsyncgeneratorresolveV8V8V8, V1));
    GateRef v2 = Int32Argument(PARAM_INDEX(BaselineAsyncgeneratorresolveV8V8V8, V2));

    GateRef frame = GetFrame(sp);
    GateRef curMethod = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);
    GateRef constpool = GetConstpoolFromMethod(curMethod);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(GetFunctionFromFrame(frame));
    GateRef acc = GetAccFromFrame(frame);
    ProfileOperation callback;

    GateRef asyncGenerator = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));
    GateRef flag = GetVregValue(sp, ZExtInt8ToPtr(v2));

    auto env = GetEnvironment();
    METHOD_EXIT();
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label updateHotness(env);
    Label isStable(env);
    Label tryContinue(env);
    Label dispatch(env);
    Label slowPath(env);

    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncGeneratorResolve),
                              { asyncGenerator, value, flag });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        Return();
    }
    Bind(&notException);
    varAcc = res;
    Branch(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &isStable);
    Bind(&isStable);
    {
        Branch(ProfilerStubBuilder(env).IsProfileTypeInfoDumped(*varProfileTypeInfo, callback), &tryContinue,
            &updateHotness);
    }
    Bind(&updateHotness);
    {
        GateRef function = GetFunctionFromFrame(frame);
        GateRef thisMethod = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, thisMethod, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef curFunc = GetFunctionFromFrame(frame);
    CallNGCRuntime(glue, RTSTUB_ID(EndCallTimer), { glue, curFunc });
#endif
    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    GateRef varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        GateRef result = CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc, *varSp, currentSp });
        varAcc = result;
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        GateRef function = GetFunctionFromFrame(prevState);
        GateRef thisMethod = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(thisMethod);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(thisMethod);
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
        GateRef result = CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                                        { glue, currentSp, varPc, *varConstpool, *varProfileTypeInfo,
                                        *varAcc, *varHotnessCounter, jumpSize });
        varAcc = result;
        Return();
    }
}

void BaselineCreateemptyobjectStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateemptyobject, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCreateemptyobject, ACC));

    NewObjectStubBuilder newBuilder(this);
    GateRef result = newBuilder.CreateEmptyObject(glue);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = result;
    Return(result);
}

void BaselineCreateemptyarrayImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateemptyarrayImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateemptyarrayImm8, SP));
    GateRef traceId = Int32Argument(PARAM_INDEX(BaselineCreateemptyarrayImm8, TRACE_ID));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineCreateemptyarrayImm8, PROFILE_TYPE_INFO));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreateemptyarrayImm8, SLOTID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    ProfileOperation callback;
    NewObjectStubBuilder newBuilder(this);
    GateRef result = newBuilder.CreateEmptyArray(glue, func, { 0, traceId, false }, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineCreateemptyarrayImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateemptyarrayImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateemptyarrayImm16, GLUE));
    GateRef traceId = Int32Argument(PARAM_INDEX(BaselineCreateemptyarrayImm16, TRACE_ID));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineCreateemptyarrayImm16, PROFILE_TYPE_INFO));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreateemptyarrayImm16, SLOTID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    ProfileOperation callback;
    NewObjectStubBuilder newBuilder(this);
    GateRef result =
        newBuilder.CreateEmptyArray(glue, func, { 0, traceId, false }, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineCreategeneratorobjV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreategeneratorobjV8, GLUE));
    GateRef genFunc = TaggedArgument(PARAM_INDEX(BaselineCreategeneratorobjV8, GEN_FUNC));

    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateGeneratorObj), { genFunc });
    Return(result);
}

void BaselineCreateiterresultobjV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateiterresultobjV8V8, GLUE));
    GateRef value = TaggedArgument(PARAM_INDEX(BaselineCreateiterresultobjV8V8, VALUE));
    GateRef flag = TaggedArgument(PARAM_INDEX(BaselineCreateiterresultobjV8V8, FLAG));

    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateIterResultObj), { value, flag });
    Return(result);
}

void BaselineCreateobjectwithexcludedkeysImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, GLUE));
    GateRef numKeys = Int32Argument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, NUMKEYS));
    GateRef obj = TaggedArgument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, OBJ));
    GateRef firstArgRegIdx =
        Int32Argument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, FIRST_ARG_REG_IDX));

    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithExcludedKeys),
        { Int16ToTaggedInt(numKeys), obj, Int16ToTaggedInt(firstArgRegIdx) });
    Return(res);
}

void BaselineCallthis0Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthis0Imm8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallthis0Imm8V8, ACC));
    GateRef thisValue = TaggedArgument(PARAM_INDEX(BaselineCallthis0Imm8V8, THIS_VALUE));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallthis0Imm8V8, HOTNESS_COUNTER));
    ProfileOperation callback;

    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG0);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLTHIS0_IMM8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_THIS_ARG0, { thisValue }, callback);
    Return(res);
}

void BaselineCreatearraywithbufferImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, SP));
    GateRef traceId = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, TRACE_ID));
    GateRef profileTypeInfo =
        TaggedPointerArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, PROFILE_TYPE_INFO));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, IMM));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, SLOTID));

    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    ProfileOperation callback;
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, currentFunc, { 0, traceId, false }, profileTypeInfo, slotId, callback);
    Return(res);
}

void BaselineCreatearraywithbufferImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, SP));
    GateRef traceId = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, TRACE_ID));
    GateRef profileTypeInfo =
        TaggedPointerArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, PROFILE_TYPE_INFO));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, IMM));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, SLOTID));

    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    ProfileOperation callback;
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, currentFunc, { 0, traceId, false }, profileTypeInfo, slotId, callback);
    Return(res);
}

void BaselineCallthis1Imm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, ACC));
    GateRef thisValue = TaggedArgument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, THIS_VALUE));
    GateRef a0Value = TaggedArgument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, A0_VALUE));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, HOTNESS_COUNTER));
    ProfileOperation callback;

    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG1);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLTHIS1_IMM8_V8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_THIS_ARG1, { a0Value, thisValue }, callback);
    Return(res);
}

void BaselineCallthis2Imm8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, SP));
    GateRef thisValue = TaggedArgument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, THIS_VALUE));
    GateRef a0Value = TaggedArgument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, A0_VALUE));
    GateRef a1Value = TaggedArgument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, A1_VALUE));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef curMethod = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);

    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS2);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLTHIS2_IMM8_V8_V8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_THIS_ARG2, { a0Value, a1Value, thisValue }, callback);
    Return(res);
}

void BaselineCreateobjectwithbufferImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithbufferImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithbufferImm8Id16, SP));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineCreateobjectwithbufferImm8Id16, IMM));

    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(currentFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef currentEnv = GetEnvFromFrame(GetFrame(sp));
    GateRef module = GetModuleFromFunction(currentFunc);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, ZExtInt16ToInt32(imm), module);
    ProfileOperation callback;
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectHavingMethod), { result, currentEnv });
    callback.ProfileCreateObject(res);
    Return(res);
}

void BaselineCreateobjectwithbufferImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithbufferImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithbufferImm16Id16, SP));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineCreateobjectwithbufferImm16Id16, IMM));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef currentEnv = GetEnvFromFrame(GetFrame(sp));
    GateRef module = GetModuleFromFunction(func);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, ZExtInt16ToInt32(imm), module);
    ProfileOperation callback;
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectHavingMethod), { result, currentEnv });
    callback.ProfileCreateObject(res);
    Return(res);
}

void BaselineCreateregexpwithliteralImm8Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm8Id16Imm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm8Id16Imm8, SP));
    GateRef stringId = TaggedArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm8Id16Imm8, STRING_ID));
    GateRef flags = TaggedArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm8Id16Imm8, FLAGS));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef pattern = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateRegExpWithLiteral),
                              { pattern, Int8ToTaggedInt(flags) });
    Return(res);
}

void BaselineCreateregexpwithliteralImm16Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm16Id16Imm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm16Id16Imm8, SP));
    GateRef stringId = TaggedArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm16Id16Imm8, STRING_ID));
    GateRef flags = TaggedArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm16Id16Imm8, FLAGS));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef pattern = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateRegExpWithLiteral),
                              { pattern, Int8ToTaggedInt(flags) });
    Return(res);
}

void BaselineNewobjapplyImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewobjapplyImm8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineNewobjapplyImm8V8, ACC));
    GateRef func = TaggedArgument(PARAM_INDEX(BaselineNewobjapplyImm8V8, FUNC));

    GateRef result = CallRuntime(glue, RTSTUB_ID(NewObjApply), { func, acc }); // acc is array
    Return(result);
}

void BaselineNewobjapplyImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewobjapplyImm16V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineNewobjapplyImm16V8, ACC));
    GateRef func = TaggedArgument(PARAM_INDEX(BaselineNewobjapplyImm16V8, FUNC));

    GateRef result = CallRuntime(glue, RTSTUB_ID(NewObjApply), { func, acc }); // acc is array
    Return(result);
}

void BaselineNewlexenvImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewlexenvImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineNewlexenvImm8, ACC));
    GateRef numVars = Int32Argument(PARAM_INDEX(BaselineNewlexenvImm8, NUM_VARS));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewlexenvImm8, SP));
    auto parent = GetEnvFromFrame(GetFrame(sp));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    auto env = GetEnvironment();

    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterNew(env);
    newBuilder.NewLexicalEnv(&result, &afterNew, ZExtInt16ToInt32(numVars), parent);
    Bind(&afterNew);
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*result, &notException);
    Bind(&notException);
    varAcc = *result;
    SetEnvToFrame(glue, GetFrame(sp), *result);
    Return(*varAcc);
}

void BaselineNewlexenvwithnameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewlexenvwithnameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewlexenvwithnameImm8Id16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineNewlexenvwithnameImm8Id16, ACC));
    GateRef numVars = Int32Argument(PARAM_INDEX(BaselineNewlexenvwithnameImm8Id16, NUM_VARS));
    GateRef scopeId = Int32Argument(PARAM_INDEX(BaselineNewlexenvwithnameImm8Id16, SCOPEID));

    auto env = GetEnvironment();
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewLexicalEnvWithName),
                              { Int16ToTaggedInt(numVars), Int16ToTaggedInt(scopeId) });
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = res;
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
    Bind(&notException);
    GateRef state = GetFrame(sp);
    SetEnvToFrame(glue, state, res);
    Return(*varAcc);
}

void BaselineAdd2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineAdd2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Add(glue, left, acc, callback);
    Return(result);
}

void BaselineSub2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineSub2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Sub(glue, left, acc, callback);
    Return(result);
}

void BaselineMul2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineMul2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Mul(glue, left, acc, callback);
    Return(result);
}

void BaselineDiv2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineDiv2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Div(glue, left, acc, callback);
    Return(result);
}

void BaselineMod2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineMod2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Mod(glue, left, acc, callback);
    Return(result);
}

void BaselineEqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineEqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Equal(glue, left, acc, callback);
    Return(result);
}

void BaselineNoteqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineNoteqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.NotEqual(glue, left, acc, callback);
    Return(result);
}

void BaselineLessImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineLessImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Less(glue, left, acc, callback);
    Return(result);
}

void BaselineLesseqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineLesseqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.LessEq(glue, left, acc, callback);
    Return(result);
}

void BaselineGreaterImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineGreaterImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Greater(glue, left, acc, callback);
    Return(result);
}

void BaselineGreatereqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineGreatereqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.GreaterEq(glue, left, acc, callback);
    Return(result);
}

void BaselineShl2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineShl2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Shl(glue, left, acc, callback);
    Return(result);
}

void BaselineShr2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineShr2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Shr(glue, left, acc, callback);
    Return(result);
}

void BaselineAshr2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineAshr2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Ashr(glue, left, acc, callback);
    Return(result);
}

void BaselineAnd2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineAnd2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.And(glue, left, acc, callback);
    Return(result);
}

void BaselineOr2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineOr2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Or(glue, left, acc, callback);
    Return(result);
}

void BaselineXor2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineXor2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Xor(glue, left, acc, callback);
    Return(result);
}

void BaselineExpImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineExpImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineExpImm8V8, SP));
    GateRef base = TaggedArgument(PARAM_INDEX(BaselineExpImm8V8, BASE));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineExpImm8V8, SLOT_ID));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);

    ProfileOperation callback(
        [this, glue, func, slotId, profileTypeInfo](const std::initializer_list<GateRef> &values,
                                                    OperationType type) {
            ProfilerStubBuilder profiler(this);
            profiler.PGOProfiler(glue, func, profileTypeInfo, slotId, values, type);
        }, nullptr);

    GateRef result = CallRuntime(glue, RTSTUB_ID(Exp), { base, acc });
    Return(result);
}

void BaselineTypeofImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTypeofImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTypeofImm8, ACC));

    GateRef result = FastTypeOf(glue, acc);
    Return(result);
}

void BaselineTypeofImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTypeofImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTypeofImm16, ACC));

    GateRef result = FastTypeOf(glue, acc);
    Return(result);
}

void BaselineTonumberImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTonumberImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTonumberImm8, ACC));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef value = *varAcc;
    Label valueIsNumber(env);
    Label valueNotNumber(env);
    Branch(TaggedIsNumber(value), &valueIsNumber, &valueNotNumber);
    Bind(&valueIsNumber);
    {
        Return(value);
    }
    Bind(&valueNotNumber);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumber), { value });
        Return(res);
    }
}

void BaselineTonumericImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTonumericImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTonumericImm8, ACC));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef value = *varAcc;
    Label valueIsNumeric(env);
    Label valueNotNumeric(env);
    Branch(TaggedIsNumeric(value), &valueIsNumeric, &valueNotNumeric);
    Bind(&valueIsNumeric);
    {
        Return(value);
    }
    Bind(&valueNotNumeric);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumeric), { value });
        Return(res);
    }
}

void BaselineNegImm8StubBuilder::GenerateCircuit()
{
    DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineNegImm8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Neg(glue, acc, callback);
    Return(result);
}

void BaselineNotImm8StubBuilder::GenerateCircuit()
{
    DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineNotImm8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Not(glue, acc, callback);
    Return(result);
}

void BaselineIncImm8StubBuilder::GenerateCircuit()
{
    DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineIncImm8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Inc(glue, acc, callback);
    Return(result);
}

void BaselineDecImm8StubBuilder::GenerateCircuit()
{
    DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineDecImm8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Dec(glue, acc, callback);
    Return(result);
}

void BaselineIsinImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineIsinImm8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineIsinImm8V8, ACC));
    GateRef prop = TaggedArgument(PARAM_INDEX(BaselineIsinImm8V8, PROP));
    ProfileOperation callback;

    GateRef result = CallRuntime(glue, RTSTUB_ID(IsIn), { prop, acc }); // acc is obj
    Return(result);
}

void BaselineInstanceofImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineInstanceofImm8V8, GLUE));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineInstanceofImm8V8, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineInstanceofImm8V8, ACC));
    GateRef obj = TaggedArgument(PARAM_INDEX(BaselineInstanceofImm8V8, OBJ));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineInstanceofImm8V8, SLOTID));
    ProfileOperation callback;

    GateRef target = acc;
    AccessObjectStubBuilder builder(this);
    GateRef result = InstanceOf(glue, obj, target, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineStrictnoteqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineStrictnoteqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.StrictNotEqual(glue, left, acc, callback);
    Return(result);
}

void BaselineStricteqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineStricteqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.StrictEqual(glue, left, acc, callback);
    Return(result);
}

void BaselineIstrueStubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineIstrue, ACC));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastToBooleanBaseline(*varAcc, true);
    Return(*varAcc);
}

void BaselineCallRuntimeIstruePrefImm8StubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeIstruePrefImm8, ACC));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastToBooleanBaseline(*varAcc, true);
    Return(*varAcc);
}

void BaselineIsfalseStubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineIsfalse, ACC));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastToBooleanBaseline(*varAcc, false);
    Return(*varAcc);
}

void BaselineCallRuntimeIsfalsePrefImm8StubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeIsfalsePrefImm8, ACC));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastToBooleanBaseline(*varAcc, false);
    Return(*varAcc);
}


void BaselineCallthis3Imm8V8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, SP));
    GateRef thisValue = TaggedArgument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, THIS_VALUE));
    GateRef a0Value = TaggedArgument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, ARG0));
    GateRef a1Value = TaggedArgument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, ARG1));
    GateRef a2Value = TaggedArgument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, ARG2));
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS3);
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef curMethod = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);

    GateRef func = acc;
    METHOD_ENTRY(func);

    GateRef jumpSize = INT_PTR(CALLTHIS3_IMM8_V8_V8_V8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_THIS_ARG3, { a0Value, a1Value, a2Value, thisValue },
                                            callback);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = res;
    Return(res);
}

void BaselineCallthisrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, ACC));
    GateRef actualNumArgs = Int32Argument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, ACTUAL_NUM_ARGS));
    GateRef thisReg = Int32Argument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, THIS_REG));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, HOTNESS_COUNTER));
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    ProfileOperation callback;

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef func = acc;
    METHOD_ENTRY(func);

    GateRef thisValue = GetVregValue(sp, ZExtInt8ToPtr(thisReg));
    GateRef argv = PtrAdd(sp, PtrMul(PtrAdd(ZExtInt8ToPtr(thisReg), IntPtr(1)), IntPtr(8)));
    GateRef jumpSize = INT_PTR(CALLTHISRANGE_IMM8_IMM8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_THIS_WITH_ARGV, { numArgs, argv, thisValue }, callback);
    varAcc = res;
    Return(res);
}

void BaselineSupercallthisrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, ACC));
    GateRef range = Int32Argument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, RANGE));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, V0));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, HOTNESS_COUNTER));
    ProfileOperation callback;

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef actualNumArgs = ZExtInt16ToInt32(range);
    GateRef thisFunc = GetFunctionFromFrame(GetFrame(sp));
    GateRef newTarget = GetNewTarget(sp);
    GateRef superCtor = GetPrototype(glue, thisFunc);

    Label ctorIsHeapObject(env);
    Label ctorIsJSFunction(env);
    Label ctorIsConstructor(env);
    Label fastPath(env);
    Label slowPath(env);
    Label checkResult(env);
    Label threadCheck(env);
    Label dispatch(env);
    Label ctorIsBase(env);
    Label ctorNotBase(env);
    Label isException(env);

    Branch(TaggedIsHeapObject(superCtor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    Branch(IsJSFunction(superCtor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    Branch(IsConstructor(superCtor), &ctorIsConstructor, &slowPath);
    Bind(&ctorIsConstructor);
    Branch(TaggedIsUndefined(newTarget), &slowPath, &fastPath);
    Bind(&fastPath);
    {
        Branch(IsBase(superCtor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            NewObjectStubBuilder newBuilder(this);
            thisObj = newBuilder.FastSuperAllocateThis(glue, superCtor, newTarget);
            Branch(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(ZExtInt16ToPtr(v0), IntPtr(JSTaggedValue::TaggedTypeSize()))); // skip function
        GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::IMM8_IMM8_V8));
        METHOD_ENTRY_ENV_DEFINED(superCtor);
        res = JSCallDispatchForBaseline(glue, superCtor, actualNumArgs, jumpSize, hotnessCounter,
                                        JSCallMode::SUPER_CALL_WITH_ARGV,
                                        { thisFunc, Int16ToTaggedInt(v0), ZExtInt32ToPtr(actualNumArgs),
                                        argv, *thisObj, newTarget }, callback);
        Jump(&threadCheck);
    }
    Bind(&slowPath);
    res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { thisFunc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
    Jump(&checkResult);
    Bind(&checkResult);
    {
        Branch(TaggedIsException(*res), &isException, &dispatch);
    }
    Bind(&threadCheck);
    {
        Branch(HasPendingException(glue), &isException, &dispatch);
    }
    Bind(&isException);
    {
        Return(*res);
    }
    Bind(&dispatch);
    varAcc = *res;
    Return(*res);
}

void BaselineSupercallarrowrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, ACC));
    GateRef range = Int32Argument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, RANGE));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, V0));

    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { acc, Int16ToTaggedInt(v0), Int8ToTaggedInt(range) });
    Return(res);
}

void BaselineDefinefuncImm8Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, ACC));
    GateRef methodId = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, METHODID));
    GateRef length = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, LENGTH));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, SP));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    NewObjectStubBuilder newBuilder(this);
    GateRef result = newBuilder.NewJSFunction(glue, constpool, ZExtInt16ToInt32(methodId));
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    {
        SetLengthToFunction(glue, result, ZExtInt8ToInt32(length));
        auto frame = GetFrame(sp);
        GateRef envHandle = GetEnvFromFrame(frame);
        SetLexicalEnvToFunction(glue, result, envHandle);
        GateRef currentFunc = GetFunctionFromFrame(frame);
        SetModuleToFunction(glue, result, GetModuleFromFunction(currentFunc));
        SetHomeObjectToFunction(glue, result, GetHomeObjectFromFunction(currentFunc));
        callback.ProfileDefineClass(result);
        varAcc = result;
        Return(result);
    }
}

void BaselineDefinefuncImm16Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, ACC));
    GateRef methodId = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, METHODID));
    GateRef length = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, LENGTH));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, SP));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    NewObjectStubBuilder newBuilder(this);
    GateRef result = newBuilder.NewJSFunction(glue, constpool, ZExtInt16ToInt32(methodId));
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    {
        SetLengthToFunction(glue, result, ZExtInt8ToInt32(length));
        auto frame = GetFrame(sp);
        GateRef envHandle = GetEnvFromFrame(frame);
        SetLexicalEnvToFunction(glue, result, envHandle);
        GateRef currentFunc = GetFunctionFromFrame(frame);
        SetModuleToFunction(glue, result, GetModuleFromFunction(currentFunc));
        SetHomeObjectToFunction(glue, result, GetHomeObjectFromFunction(currentFunc));
        varAcc = result;
        callback.ProfileDefineClass(result);
        Return(result);
    }
}

void BaselineDefinemethodImm8Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinemethodImm8Id16Imm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDefinemethodImm8Id16Imm8, ACC));
    GateRef methodId = Int32Argument(PARAM_INDEX(BaselineDefinemethodImm8Id16Imm8, METHODID));
    GateRef length = Int32Argument(PARAM_INDEX(BaselineDefinemethodImm8Id16Imm8, LENGTH));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinemethodImm8Id16Imm8, SP));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef lexEnv = GetEnvFromFrame(GetFrame(sp));
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetMethodFromConstPool(glue, constpool, ZExtInt16ToInt32(methodId)));
    result = CallRuntime(glue, RTSTUB_ID(DefineMethod), { *result, acc, Int8ToTaggedInt(length),
                                                          lexEnv, GetModule(sp) });
    (void) env;
    (void) length;
    Return(*result);
}

void BaselineDefinemethodImm16Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinemethodImm16Id16Imm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDefinemethodImm16Id16Imm8, ACC));
    GateRef methodId = Int32Argument(PARAM_INDEX(BaselineDefinemethodImm16Id16Imm8, METHODID));
    GateRef length = Int32Argument(PARAM_INDEX(BaselineDefinemethodImm16Id16Imm8, LENGTH));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinemethodImm16Id16Imm8, SP));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef lexEnv = GetEnvFromFrame(GetFrame(sp));
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetMethodFromConstPool(glue, constpool, ZExtInt16ToInt32(methodId)));
    result = CallRuntime(glue, RTSTUB_ID(DefineMethod), { *result, acc, Int8ToTaggedInt(length),
                                                          lexEnv, GetModule(sp) });
    (void) env;
    (void) length;
    Return(*result);
}

void BaselineCallarg0Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallarg0Imm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallarg0Imm8, ACC));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallarg0Imm8, HOTNESS_COUNTER));
    ProfileOperation callback;

    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG0);
    GateRef jumpSize = INT_PTR(CALLARG0_IMM8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_ARG0, {}, callback);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = res;
    Return(res);
}

void BaselineSupercallspreadImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSupercallspreadImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineSupercallspreadImm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSupercallspreadImm8V8, ACC));
    GateRef array = TaggedArgument(PARAM_INDEX(BaselineSupercallspreadImm8V8, ARRARY));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineSupercallspreadImm8V8, HOTNESS_COUNTER));
    ProfileOperation callback;

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef thisFunc = acc;
    GateRef newTarget = GetNewTarget(sp);
    GateRef superCtor = GetPrototype(glue, thisFunc);

    Label dispatch(env);
    Label normalPath(env);
    Label slowPath(env);
    Label ctorIsJSFunction(env);
    Label ctorIsBase(env);
    Label ctorNotBase(env);
    Label ctorIsHeapObject(env);
    Label ctorIsConstructor(env);
    Label threadCheck(env);
    Label isException(env);

    Branch(TaggedIsHeapObject(superCtor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    Branch(IsJSFunction(superCtor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    Branch(IsConstructor(superCtor), &ctorIsConstructor, &slowPath);
    Bind(&ctorIsConstructor);
    Branch(TaggedIsUndefined(newTarget), &slowPath, &normalPath);
    Bind(&normalPath);
    {
        Branch(IsBase(superCtor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        NewObjectStubBuilder objBuilder(this);
        thisObj = objBuilder.FastSuperAllocateThis(glue, superCtor, newTarget);
        Branch(HasPendingException(glue), &isException, &ctorNotBase);
        Bind(&ctorNotBase);
        GateRef argvLen = Load(VariableType::INT32(), array, IntPtr(JSArray::LENGTH_OFFSET));
        GateRef srcElements = GetCallSpreadArgs(glue, array, callback);
        GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::IMM8_V8));
        METHOD_ENTRY_ENV_DEFINED(superCtor);
        GateRef elementsPtr = PtrAdd(srcElements, IntPtr(TaggedArray::DATA_OFFSET));
        res = JSCallDispatchForBaseline(glue, superCtor, argvLen, jumpSize, hotnessCounter,
                                        JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV,
                                        { thisFunc, array, ZExtInt32ToPtr(argvLen), elementsPtr, *thisObj, newTarget },
                                        callback);
        Jump(&threadCheck);
    }
    Bind(&slowPath);
    {
        res = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), { thisFunc, array });
        Jump(&threadCheck);
    }
    Bind(&threadCheck);
    {
        GateRef isError = BoolAnd(TaggedIsException(*res), HasPendingException(glue));
        Branch(isError, &isException, &dispatch);
    }
    Bind(&isException);
    {
        Return(*res);
    }
    Bind(&dispatch);
    {
        varAcc = *res;
        Return(*res);
    }
}

void BaselineApplyImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineApplyImm8V8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineApplyImm8V8V8, ACC));
    GateRef obj = TaggedArgument(PARAM_INDEX(BaselineApplyImm8V8V8, OBJ));
    GateRef array = TaggedArgument(PARAM_INDEX(BaselineApplyImm8V8V8, ARRARY));

    GateRef func = acc;
    GateRef res = CallRuntime(glue, RTSTUB_ID(CallSpread), { func, obj, array });
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = res;
    Return(res);
}

void BaselineCallargs2Imm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, ACC));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, HOTNESS_COUNTER));
    GateRef a0Value = TaggedArgument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, A0_VALUE));
    GateRef a1Value = TaggedArgument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, A1_VALUE));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS2);
    ProfileOperation callback;

    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLARGS2_IMM8_V8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_ARG2, { a0Value, a1Value }, callback);
    varAcc = res;
    Return(res);
}

void BaselineCallargs3Imm8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, SP));
    GateRef a0Value = TaggedArgument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, A0_VALUE));
    GateRef a1Value = TaggedArgument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, A1_VALUE));
    GateRef a2Value = TaggedArgument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, A2_VALUE));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef curMethod = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS3);
    ProfileOperation callback;

    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLARGS3_IMM8_V8_V8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_ARG3, { a0Value, a1Value, a2Value }, callback);
    varAcc = res;
    Return(res);
}

void BaselineCallrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, ACC));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, HOTNESS_COUNTER));
    GateRef actualNumArgs = Int32Argument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, ACTUAL_NUM_ARGS));
    GateRef argStart = Int32Argument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, ARG_START));
    ProfileOperation callback;

    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef argv = PtrAdd(sp, PtrMul(ZExtInt8ToPtr(argStart), IntPtr(8)));
    GateRef jumpSize = INT_PTR(CALLRANGE_IMM8_IMM8_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_WITH_ARGV, { numArgs, argv }, callback);
    Return(res);
}

void BaselineLdexternalmodulevarImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdexternalmodulevarImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdexternalmodulevarImm8, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineLdexternalmodulevarImm8, INDEX));

    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdExternalModuleVarByIndex), { Int8ToTaggedInt(index) });
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = moduleRef;
    Return(moduleRef);
}

void BaselineLdthisbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdthisbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdthisbynameImm8Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdthisbynameImm8Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdthisbynameImm8Id16, SLOT_ID));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetThisFromFrame(GetFrame(sp));
    GateRef acc = GetAccFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(GetFunctionFromFrame(frame));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, stringIdInfo, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineDefinegettersetterbyvalueV8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, SP));
    GateRef prop = TaggedArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, PROP));
    GateRef getter = TaggedArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, GETTER));
    GateRef setter = TaggedArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, SETTER));
    GateRef offset = Int32Argument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, OFFSET));
    // passed by stack
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, ACC));
    GateRef obj = TaggedArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, OBJ));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef offsetPtr = TaggedPtrToTaggedIntPtr(IntPtr(offset));
    GateRef res = CallRuntime(glue, RTSTUB_ID(DefineGetterSetterByValue),
                              { obj, prop, getter, setter, acc, func, offsetPtr }); // acc is flag
    Return(res);
}

void BaselineLdthisbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdthisbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdthisbynameImm16Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdthisbynameImm16Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdthisbynameImm16Id16, SLOT_ID));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetThisFromFrame(GetFrame(sp));
    GateRef acc = GetAccFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(GetFunctionFromFrame(frame));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, stringIdInfo, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineStthisbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStthisbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStthisbynameImm8Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStthisbynameImm8Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStthisbynameImm8Id16, SLOT_ID));

    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(GetFunctionFromFrame(frame));
    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(GetFrame(sp));
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result =
        builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);
    (void)result;
    Return();
}

void BaselineStthisbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStthisbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStthisbynameImm16Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStthisbynameImm16Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStthisbynameImm16Id16, SLOT_ID));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(GetFunctionFromFrame(frame));
    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(GetFrame(sp));
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);
    (void) result;
    Return();
}

void BaselineLdthisbyvalueImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdthisbyvalueImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdthisbyvalueImm8, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineLdthisbyvalueImm8, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdthisbyvalueImm8, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdthisbyvalueImm8, SLOT_ID));
    ProfileOperation callback;

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = acc;

    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(GetFrame(sp));
    GateRef result = builder.LoadObjByValue(glue, receiver, propKey, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineLdthisbyvalueImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdthisbyvalueImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdthisbyvalueImm16, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineLdthisbyvalueImm16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdthisbyvalueImm16, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdthisbyvalueImm16, SLOT_ID));
    ProfileOperation callback;

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = acc;

    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(GetFrame(sp));
    GateRef result = builder.LoadObjByValue(glue, receiver, propKey, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineStthisbyvalueImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, SLOT_ID));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, PROP_KEY));
    ProfileOperation callback;

    GateRef value = acc;
    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(GetFrame(sp));
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, value, profileTypeInfo, slotId, callback);
    (void) result;
    Return();
}

void BaselineStthisbyvalueImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, SLOT_ID));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, PROP_KEY));
    ProfileOperation callback;

    GateRef value = acc;
    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(GetFrame(sp));
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, value, profileTypeInfo, slotId, callback);
    (void) result;
    Return();
}

void BaselineDynamicimportStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDynamicimport, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDynamicimport, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDynamicimport, ACC));

    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef specifier = *varAcc;
    GateRef res = CallRuntime(glue, RTSTUB_ID(DynamicImport), { specifier, currentFunc });
    Return(res);
}

void BaselineDefineclasswithbufferImm8Id16Id16Imm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefineclasswithbufferImm8Id16Id16Imm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefineclasswithbufferImm8Id16Id16Imm16V8, SP));
    GateRef methodId = Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm8Id16Id16Imm16V8, METHOD_ID));
    GateRef literalId = Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm8Id16Id16Imm16V8, LITERRAL_ID));
    GateRef length = Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm8Id16Id16Imm16V8, LENGTH));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm8Id16Id16Imm16V8, V0));
    ProfileOperation callback;
    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v0));

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);

    auto env = GetEnvironment();
    GateRef lexicalEnv = GetEnvFromFrame(GetFrame(sp));
    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    GateRef module = GetModuleFromFunction(currentFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer),
                              { proto, lexicalEnv, constpool,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId), module,
                                Int16ToTaggedInt(length)});

    Label isException(env);
    Label isNotException(env);
    Branch(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        Return(acc);
    }
    Bind(&isNotException);
    callback.ProfileDefineClass(res);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = res;
    Return(*varAcc);
}

void BaselineDefineclasswithbufferImm16Id16Id16Imm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, SP));
    GateRef methodId = Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, METHOD_ID));
    GateRef literalId = Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, LITERRAL_ID));
    GateRef length = Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, LENGTH));
    GateRef proto = TaggedArgument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, PROTO));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef lexicalEnv = GetEnvFromFrame(GetFrame(sp));
    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    GateRef module = GetModuleFromFunction(currentFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer),
                              { proto, lexicalEnv, constpool,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId), module,
                                Int16ToTaggedInt(length)});

    Label isException(env);
    Label isNotException(env);
    Branch(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        Return(res);
    }
    Bind(&isNotException);
    callback.ProfileDefineClass(res);
    Return(res);
}

void BaselineResumegeneratorStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineResumegenerator, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineResumegenerator, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineResumegenerator, ACC));
    (void) sp;
    (void) glue;

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef obj = *varAcc;

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef frame = GetFrame(sp);
    GateRef curFunc = GetFunctionFromFrame(frame);
    CallNGCRuntime(glue, RTSTUB_ID(StartCallTimer), { glue, curFunc, False() });
#endif
    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    Branch(TaggedIsAsyncGeneratorObject(obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    Bind(&isAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), obj, resumeResultOffset);
        Jump(&dispatch);
    }
    Bind(&notAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), obj, resumeResultOffset);
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Return(*varAcc);
}

void BaselineGetresumemodStubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetresumemod, ACC));
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef obj = *varAcc;

    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    Branch(TaggedIsAsyncGeneratorObject(obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    Bind(&isAsyncGeneratorObj);
    {
        varAcc = IntToTaggedPtr(GetResumeModeFromAsyncGeneratorObject(obj));
        Jump(&dispatch);
    }
    Bind(&notAsyncGeneratorObj);
    {
        varAcc = IntToTaggedPtr(GetResumeModeFromGeneratorObject(obj));
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Return(*varAcc);
}

void BaselineGettemplateobjectImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGettemplateobjectImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGettemplateobjectImm8, ACC));

    GateRef literal = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
    Return(result);
}

void BaselineGettemplateobjectImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGettemplateobjectImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGettemplateobjectImm16, ACC));

    GateRef literal = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
    Return(result);
}

void BaselineGetnextpropnameV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef iter = TaggedArgument(SECOND_PARAMETER);

    GateRef result = NextInternal(glue, iter);
    Return(result);
}

void BaselineJeqzImm8StubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(FIRST_PARAMETER);
    GateRef offset = Int32Argument(SECOND_PARAMETER);
    GateRef profileTypeInfo = TaggedPointerArgument(THIRD_PARAMETER);
    GateRef hotnessCounter = Int32Argument(FOURTH_PARAMETER);
    (void) offset;
    ProfileOperation callback;

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(TaggedIsFalse(acc), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        Label dispatch(env);
        Label slowPath(env);
        Return();
    }
    Bind(&last);
    Return();
}

void BaselineJeqzImm16StubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(FIRST_PARAMETER);
    GateRef offset = Int32Argument(SECOND_PARAMETER);
    GateRef profileTypeInfo = TaggedPointerArgument(THIRD_PARAMETER);
    GateRef hotnessCounter = Int32Argument(FOURTH_PARAMETER);
    (void) offset;
    ProfileOperation callback;

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(TaggedIsFalse(acc), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        Label dispatch(env);
        Label slowPath(env);
        Return();
    }
    Bind(&last);
    Return();
}

void BaselineSetobjectwithprotoImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm8V8, ACC));
    GateRef proto = TaggedArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm8V8, PROTO));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef obj = *varAcc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, obj });
    (void) result;
    (void) env;
    Return();
}

void BaselineDelobjpropV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef obj = TaggedArgument(THIRD_PARAMETER);

    GateRef prop = acc;
    GateRef result = DeletePropertyOrThrow(glue, obj, prop);
    (void) result;
    Return();
}

void BaselineAsyncfunctionawaituncaughtV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef asyncFuncObj = TaggedArgument(THIRD_PARAMETER);

    GateRef value = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncFunctionAwaitUncaught), { asyncFuncObj, value });
    Return(result);
}

void BaselineCopydatapropertiesV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef dst = TaggedArgument(THIRD_PARAMETER);

    GateRef src = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(CopyDataProperties), { dst, src });
    Return(result);
}

void BaselineStarrayspreadV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef dst = TaggedArgument(THIRD_PARAMETER);
    GateRef index = TaggedArgument(FOURTH_PARAMETER);

    GateRef result = CallRuntime(glue, RTSTUB_ID(StArraySpread), { dst, index, acc }); // acc is res
    Return(result);
}

void BaselineSetobjectwithprotoImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef proto = TaggedArgument(THIRD_PARAMETER);

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef obj = *varAcc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, obj });
    (void) result;
    (void) env;
    Return();
}

void BaselineLdobjbyvalueImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, GLUE));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, SLOTID));
    ProfileOperation callback;

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = acc;
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.LoadObjByValue(glue, receiver, propKey, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineLdobjbyvalueImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, GLUE));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, SLOTID));
    ProfileOperation callback;

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = acc;
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.LoadObjByValue(glue, receiver, propKey, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineStobjbyvalueImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, GLUE));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, SLOTID));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, PROP_KEY));
    ProfileOperation callback;

    GateRef value = acc;

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, value, profileTypeInfo, slotId, callback);
    (void) result;
    Return();
}

void BaselineStobjbyvalueImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, GLUE));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, SLOTID));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, PROP_KEY));
    ProfileOperation callback;

    GateRef value = acc;

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, value, profileTypeInfo, slotId, callback);
    (void) result;
    Return();
}

void BaselineStownbyvalueImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef receiver = TaggedArgument(THIRD_PARAMETER);
    GateRef propKey = TaggedArgument(FOURTH_PARAMETER);
    ProfileOperation callback;

    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    Label notClassConstructor(env);
    Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    Bind(&notClassConstructor);
    Label notClassPrototype(env);
    Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        GateRef result = SetPropertyByValue(glue, receiver, propKey, acc, true, callback); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return();
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByValue), { receiver, propKey, acc });
        (void) result;
        Return();
    }
}

void BaselineStownbyvalueImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef receiver = TaggedArgument(THIRD_PARAMETER);
    GateRef propKey = TaggedArgument(FOURTH_PARAMETER);
    ProfileOperation callback;

    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    Label notClassConstructor(env);
    Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    Bind(&notClassConstructor);
    Label notClassPrototype(env);
    Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        GateRef result = SetPropertyByValue(glue, receiver, propKey, acc, true, callback); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return();
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByValue), { receiver, propKey, acc });
        (void) result;
        Return();
    }
}

void BaselineLdsuperbyvalueImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm8V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm8V8, RECEIVER));

    GateRef propKey = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, propKey }); // sp for thisFunc
    Return(result);
}

void BaselineLdsuperbyvalueImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm16V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm16V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm16V8, RECEIVER));

    GateRef propKey = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, propKey }); // sp for thisFunc
    Return(result);
}

void BaselineStsuperbyvalueImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStsuperbyvalueImm8V8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm8V8V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm8V8V8, RECEIVER));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm8V8V8, PROP_KEY));
    ProfileOperation callback;

     // acc is value, sp for thisFunc
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    (void) result;
    Return();
}

void BaselineStsuperbyvalueImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStsuperbyvalueImm16V8V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm16V8V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm16V8V8, RECEIVER));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm16V8V8, PROP_KEY));
    ProfileOperation callback;

     // acc is value, sp for thisFunc
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    (void) result;
    Return();
}

void BaselineLdobjbyindexImm8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, INDEX));
    ProfileOperation callback;

    auto env = GetEnvironment();

    GateRef receiver = acc;
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, receiver, index, callback);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        Return(result);
    }
}

void BaselineLdobjbyindexImm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, INDEX));
    ProfileOperation callback;

    auto env = GetEnvironment();

    GateRef receiver = acc;
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, receiver, index, callback);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        Return(result);
    }
}

void BaselineStobjbyindexImm8V8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbyindexImm8V8Imm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStobjbyindexImm8V8Imm16, ACC));
    GateRef receiver = Int32Argument(PARAM_INDEX(BaselineStobjbyindexImm8V8Imm16, RECEIVER));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStobjbyindexImm8V8Imm16, INDEX));

    auto env = GetEnvironment();
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = SetPropertyByIndex(glue, receiver, index, acc, false);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return();
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        (void) result;
        Return();
    }
}

void BaselineStobjbyindexImm16V8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, ACC));
    GateRef receiver = Int32Argument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, RECEIVER));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, INDEX));

    auto env = GetEnvironment();
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = SetPropertyByIndex(glue, receiver, index, acc, false);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return();
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        (void) result;
        Return();
    }
}

void BaselineStownbyindexImm8V8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, GLUE));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, RECEIVER));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, SLOTID));
    ProfileOperation callback;

    GateRef value = acc;

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreOwnByIndex(
        glue, receiver, ZExtInt16ToInt32(index), value, profileTypeInfo, slotId, callback);
    (void) result;
    Return();
}

void BaselineStownbyindexImm16V8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, GLUE));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, RECEIVER));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, SLOTID));
    ProfileOperation callback;

    GateRef value = acc;
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreOwnByIndex(glue, receiver, ZExtInt16ToInt32(index), value, profileTypeInfo, slotId, callback);
    (void) result;
    Return();
}

void BaselineAsyncfunctionresolveV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionresolveV8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionresolveV8, ACC));
    GateRef asyncFuncObj = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionresolveV8, ASYNC_FUNC_OBJ));

    GateRef value = acc;
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedTrue() });
    Return(res);
}

void BaselineAsyncfunctionrejectV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionrejectV8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionrejectV8, ACC));
    GateRef asyncFuncObj = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionrejectV8, ASYNC_FUNC_OBJ));

    GateRef value = acc;
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedFalse() });
    Return(res);
}

void BaselineCopyrestargsImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCopyrestargsImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCopyrestargsImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCopyrestargsImm8, ACC));
    GateRef restIdx = TaggedArgument(PARAM_INDEX(BaselineCopyrestargsImm8, REST_IDX));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    auto env = GetEnvironment();
    GateRef startIdxAndNumArgs = GetStartIdxAndNumArgs(sp, restIdx);
    GateRef startIdx = TruncInt64ToInt32(Int64LSR(startIdxAndNumArgs, Int64(32)));
    GateRef numArgs = TruncInt64ToInt32(startIdxAndNumArgs);
    Label dispatch(env);
    Label slowPath(env);
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef intialHClass = Load(VariableType::JS_ANY(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    res = newBuilder.NewJSArrayWithSize(intialHClass, numArgs);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, *res, lengthOffset, TruncInt64ToInt32(numArgs));
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    SetPropertyInlinedProps(glue, *res, intialHClass, accessor, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
    SetExtensibleToBitfield(glue, *res, true);
    Label setArgumentsBegin(env);
    Label setArgumentsAgain(env);
    Label setArgumentsEnd(env);
    GateRef elements = GetElementsArray(*res);
    Branch(Int32UnsignedLessThan(*i, numArgs), &setArgumentsBegin, &setArgumentsEnd);
    LoopBegin(&setArgumentsBegin);
    {
        GateRef idx = ZExtInt32ToPtr(Int32Add(startIdx, *i));
        GateRef receiver = Load(VariableType::JS_ANY(), sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, *i, receiver);
        i = Int32Add(*i, Int32(1));
        Branch(Int32UnsignedLessThan(*i, numArgs), &setArgumentsAgain, &setArgumentsEnd);
        Bind(&setArgumentsAgain);
    }
    LoopEnd(&setArgumentsBegin);
    Bind(&setArgumentsEnd);
    Branch(HasPendingException(glue), &slowPath, &dispatch);
    Bind(&dispatch);
    {
        varAcc = *res;
        Return(*res);
    }

    Bind(&slowPath);
    {
        GateRef result2 = CallRuntime(glue, RTSTUB_ID(CopyRestArgs), { IntToTaggedInt(restIdx) });
        Return(result2);
    }
}

void BaselineLdlexvarImm4Imm4StubBuilder::GenerateCircuit()
{
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdlexvarImm4Imm4, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdlexvarImm4Imm4, ACC));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineLdlexvarImm4Imm4, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineLdlexvarImm4Imm4, SLOT));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(GetFrame(sp)));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    varAcc = variable;
    Return(variable);
}

void BaselineStlexvarImm4Imm4StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStlexvarImm4Imm4, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStlexvarImm4Imm4, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStlexvarImm4Imm4, ACC));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineStlexvarImm4Imm4, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineStlexvarImm4Imm4, SLOT));

    auto env = GetEnvironment();
    GateRef value = acc;
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(GetFrame(sp)));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    Return();
}

void BaselineGetmodulenamespaceImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetmodulenamespaceImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetmodulenamespaceImm8, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineGetmodulenamespaceImm8, INDEX));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespaceByIndex), { IntToTaggedInt(index) });
    varAcc = moduleRef;
    Return(moduleRef);
}

void BaselineStmodulevarImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef index = Int32Argument(THIRD_PARAMETER);
    GateRef value = acc;

    CallRuntime(glue, RTSTUB_ID(StModuleVarByIndex), { IntToTaggedInt(index), value });
    Return();
}

void BaselineTryldglobalbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, SP));
    GateRef profileTypeInfo =
        TaggedPointerArgument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, ZExtInt16ToInt32(stringId));
    GateRef result = builder.TryLoadGlobalByName(glue, 0, info, profileTypeInfo, ZExtInt16ToInt32(slotId), callback);
    Return(result);
}

void BaselineTrystglobalbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, STRING_ID));
    ProfileOperation callback;
    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, stringId);
    GateRef result =
        builder.TryStoreGlobalByName(glue, 0, info, acc, profileTypeInfo, slotId, callback);
    (void)result;
    Return();
}

void BaselineTrystglobalbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, SP));
    GateRef profileTypeInfo =
        TaggedPointerArgument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, ZExtInt16ToInt32(stringId));
    GateRef result =
        builder.TryStoreGlobalByName(glue, 0, info, acc, profileTypeInfo, ZExtInt16ToInt32(slotId), callback);
    (void)result;
    Return();
}

void BaselineLdglobalvarImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, ZExtInt16ToInt32(stringId));
    GateRef result = builder.LoadGlobalVar(glue, 0, info, profileTypeInfo, ZExtInt16ToInt32(slotId), callback);
    Return(result);
}

void BaselineStglobalvarImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStglobalvarImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStglobalvarImm16Id16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStglobalvarImm16Id16, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStglobalvarImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStglobalvarImm16Id16, STRING_ID));
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(GetFunctionFromFrame(GetFrame(sp)));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, ZExtInt16ToInt32(stringId));
    GateRef result = builder.StoreGlobalVar(glue, 0, info, acc, profileTypeInfo, ZExtInt16ToInt32(slotId));
    (void) result;
    Return();
}

void BaselineLdobjbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef receiver = acc;
    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, stringIdInfo, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineLdobjbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, PROFILE_TYPE_INFO));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef receiver = acc;
    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, ZExtInt8ToInt32(stringId));
    GateRef result = builder.LoadObjByName(glue, receiver, 0, stringIdInfo, profileTypeInfo,
                                           ZExtInt8ToInt32(slotId), callback);
    Return(result);
}

void BaselineStobjbynameImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbynameImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStobjbynameImm8Id16V8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStobjbynameImm8Id16V8, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStobjbynameImm8Id16V8, STRING_ID));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStobjbynameImm8Id16V8, RECEIVER));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(GetFunctionFromFrame(frame));

    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result =
        builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);
    (void) result;
    Return();
}

void BaselineStobjbynameImm16Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, STRING_ID));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, RECEIVER));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(GetFunctionFromFrame(frame));
    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, ZExtInt16ToInt32(stringId));
    GateRef result = builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo,
                                            ZExtInt16ToInt32(slotId), callback);
    (void) result;
    Return();
}

void BaselineStownbynameImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, RECEIVER));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label checkResult(env);

    Label isJSObject(env);
    Label slowPath(env);
    Branch(IsJSObject(receiver), &isJSObject, &slowPath);
    Bind(&isJSObject);
    {
        Label notClassConstructor(env);
        Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Label fastPath(env);
            Branch(IsClassPrototype(receiver), &slowPath, &fastPath);
            Bind(&fastPath);
            {
                result = SetPropertyByName(glue, receiver, propKey, acc, true, True(), callback);
                Branch(TaggedIsHole(*result), &slowPath, &checkResult);
            }
        }
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(StOwnByName), { receiver, propKey, acc });
        Jump(&checkResult);
    }
    Bind(&checkResult);
    {
        (void) result;
        Return();
    }
}

void BaselineStownbynameImm16Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, RECEIVER));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label checkResult(env);

    Label isJSObject(env);
    Label slowPath(env);
    Branch(IsJSObject(receiver), &isJSObject, &slowPath);
    Bind(&isJSObject);
    {
        Label notClassConstructor(env);
        Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Label fastPath(env);
            Branch(IsClassPrototype(receiver), &slowPath, &fastPath);
            Bind(&fastPath);
            {
                result = SetPropertyByName(glue, receiver, propKey, acc, true, True(), callback);
                Branch(TaggedIsHole(*result), &slowPath, &checkResult);
            }
        }
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(StOwnByName), { receiver, propKey, acc });
        Jump(&checkResult);
    }
    Bind(&checkResult);
    {
        (void) result;
        Return();
    }
}

void BaselineLdsuperbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsuperbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdsuperbynameImm8Id16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdsuperbynameImm8Id16, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdsuperbynameImm8Id16, STRING_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef receiver = acc;
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { receiver, propKey });
    Return(result);
}

void BaselineLdsuperbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsuperbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdsuperbynameImm16Id16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdsuperbynameImm16Id16, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdsuperbynameImm16Id16, STRING_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef receiver = acc;
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { receiver, propKey });
    Return(result);
}

void BaselineStsuperbynameImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStsuperbynameImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStsuperbynameImm8Id16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStsuperbynameImm8Id16V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStsuperbynameImm8Id16V8, RECEIVER));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStsuperbynameImm8Id16V8, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    (void) result;
    Return();
}

void BaselineStsuperbynameImm16Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStsuperbynameImm16Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStsuperbynameImm16Id16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStsuperbynameImm16Id16V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStsuperbynameImm16Id16V8, RECEIVER));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStsuperbynameImm16Id16V8, STRING_ID));
    ProfileOperation callback;

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    (void) result;
    Return();
}

void BaselineLdlocalmodulevarImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdlocalmodulevarImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdlocalmodulevarImm8, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineLdlocalmodulevarImm8, INDEX));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdLocalModuleVarByIndex), { Int8ToTaggedInt(index) });
    varAcc = moduleRef;
    Return(moduleRef);
}

void BaselineStconsttoglobalrecordImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStconsttoglobalrecordImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStconsttoglobalrecordImm16Id16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStconsttoglobalrecordImm16Id16, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStconsttoglobalrecordImm16Id16, STRING_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedTrue() });
    (void) result;
    Return();
}

void BaselineJeqzImm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineJeqzImm32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineJeqzImm32, ACC));
    GateRef func = TaggedPointerArgument(PARAM_INDEX(BaselineJeqzImm32, FUNC));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineJeqzImm32, PROFILE_TYPE_INFO));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineJeqzImm32, HOTNESS_COUNTER));
    GateRef offset = Int32Argument(PARAM_INDEX(BaselineJeqzImm32, OFFSET));

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    ProfileOperation callback;

    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(TaggedIsFalse(acc), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        Label dispatch(env);
        Label slowPath(env);
        UPDATE_HOTNESS(func, callback);
        Return();
    }
    Bind(&last);
    Return();
}

void BaselineJnezImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef func = TaggedPointerArgument(THIRD_PARAMETER);
    GateRef profileTypeInfo = TaggedPointerArgument(FOURTH_PARAMETER);
    GateRef hotnessCounter = Int32Argument(FIFTH_PARAMETER);
    GateRef offset = Int8(SIXTH_PARAMETER);

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    ProfileOperation callback;

    Label accEqualTrue(env);
    Label accNotEqualTrue(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(TaggedIsTrue(acc), &accEqualTrue, &accNotEqualTrue);
    Bind(&accNotEqualTrue);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &last, &accEqualTrue);
            }
        }
    }
    Bind(&accEqualTrue);
    {
        Label dispatch(env);
        Label slowPath(env);
        UPDATE_HOTNESS(func, callback);
        Return();
    }
    Bind(&last);
    Return();
}

void BaselineJnezImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef func = TaggedPointerArgument(THIRD_PARAMETER);
    GateRef profileTypeInfo = TaggedPointerArgument(FOURTH_PARAMETER);
    GateRef hotnessCounter = Int32Argument(FIFTH_PARAMETER);
    GateRef offset = Int32Argument(SIXTH_PARAMETER);

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    ProfileOperation callback;

    Label accEqualTrue(env);
    Label accNotEqualTrue(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(TaggedIsTrue(acc), &accEqualTrue, &accNotEqualTrue);
    Bind(&accNotEqualTrue);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &last, &accEqualTrue);
            }
        }
    }
    Bind(&accEqualTrue);
    {
        Label dispatch(env);
        Label slowPath(env);
        UPDATE_HOTNESS(func, callback);
        Return();
    }
    Bind(&last);
    Return();
}

void BaselineJnezImm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef func = TaggedPointerArgument(THIRD_PARAMETER);
    GateRef profileTypeInfo = TaggedPointerArgument(FOURTH_PARAMETER);
    GateRef hotnessCounter = Int32Argument(FIFTH_PARAMETER);
    GateRef offset = Int32Argument(SIXTH_PARAMETER);

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    ProfileOperation callback;

    Label accEqualTrue(env);
    Label accNotEqualTrue(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(TaggedIsTrue(acc), &accEqualTrue, &accNotEqualTrue);
    Bind(&accNotEqualTrue);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &last, &accEqualTrue);
            }
        }
    }
    Bind(&accEqualTrue);
    {
        Label dispatch(env);
        Label slowPath(env);
        UPDATE_HOTNESS(func, callback);
        Return();
    }
    Bind(&last);
    Return();
}

void BaselineStownbyvaluewithnamesetImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef receiver = TaggedArgument(THIRD_PARAMETER);
    GateRef propKey = TaggedArgument(FOURTH_PARAMETER);

    auto env = GetEnvironment();
    ProfileOperation callback;
    Label isHeapObject(env);
    Label slowPath(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label notException(env);
    Label notException1(env);
    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    {
        Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByValue(glue, receiver, propKey, acc, true, callback);
                Branch(TaggedIsHole(res), &slowPath, &notHole);
                Bind(&notHole);
                {
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    (void)res;
                    Return();
                }
            }
        }
    }
    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, acc });
        (void)res;
        Return();
    }
}

void BaselineStownbyvaluewithnamesetImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef receiver = TaggedArgument(THIRD_PARAMETER);
    GateRef propKey = TaggedArgument(FOURTH_PARAMETER);

    auto env = GetEnvironment();
    ProfileOperation callback;
    Label isHeapObject(env);
    Label slowPath(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label notException(env);
    Label notException1(env);
    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    {
        Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByValue(glue, receiver, propKey, acc, true, callback);
                Branch(TaggedIsHole(res), &slowPath, &notHole);
                Bind(&notHole);
                {
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    (void)res;
                    Return();
                }
            }
        }
    }
    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, acc });
        (void)res;
        Return();
    }
}

void BaselineStownbynamewithnamesetImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, ACC));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, STRING_ID));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, RECEIVER));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    ProfileOperation callback;
    Label isJSObject(env);
    Label notJSObject(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label notException(env);
    Label notException1(env);
    Branch(IsJSObject(receiver), &isJSObject, &notJSObject);
    Bind(&isJSObject);
    {
        Branch(IsClassConstructor(receiver), &notJSObject, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(receiver), &notJSObject, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByName(glue, receiver, propKey, acc, true, True(), callback);
                Branch(TaggedIsHole(res), &notJSObject, &notHole);
                Bind(&notHole);
                {
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    Return();
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, acc });
        (void)res;
        Return();
    }
}

void BaselineStownbynamewithnamesetImm16Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, ACC));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, STRING_ID));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, RECEIVER));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    ProfileOperation callback;
    Label isJSObject(env);
    Label notJSObject(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label notException(env);
    Label notException1(env);
    Branch(IsJSObject(receiver), &isJSObject, &notJSObject);
    Bind(&isJSObject);
    {
        Branch(IsClassConstructor(receiver), &notJSObject, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(receiver), &notJSObject, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByName(glue, receiver, propKey, acc, true, True(), callback);
                Branch(TaggedIsHole(res), &notJSObject, &notHole);
                Bind(&notHole);
                {
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    (void)res;
                    Return();
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, acc });
        (void)res;
        Return();
    }
}

void BaselineLdbigintId16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdbigintId16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdbigintId16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdbigintId16, STRING_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef numberBigInt = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef res = CallRuntime(glue, RTSTUB_ID(LdBigInt), { numberBigInt });
    Return(res);
}

void BaselineJmpImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineJmpImm8, GLUE));
    GateRef func = TaggedPointerArgument(PARAM_INDEX(BaselineJmpImm8, FUNC));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineJmpImm8, PROFILE_TYPE_INFO));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineJmpImm8, HOTNESS_COUNTER));
    GateRef offset = Int32Argument(PARAM_INDEX(BaselineJmpImm8, OFFSET));

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    Label dispatch(env);
    Label slowPath(env);
    ProfileOperation callback;
    UPDATE_HOTNESS(func, callback);
    Return();
}

void BaselineJmpImm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef func = TaggedPointerArgument(SECOND_PARAMETER);
    GateRef profileTypeInfo = TaggedPointerArgument(THIRD_PARAMETER);
    GateRef hotnessCounter = Int32Argument(FOURTH_PARAMETER);
    GateRef offset = Int32Argument(FIFTH_PARAMETER);

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    Label dispatch(env);
    Label slowPath(env);
    ProfileOperation callback;
    UPDATE_HOTNESS(func, callback);
    Return();
}

void BaselineFldaiImm64StubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineFldaiImm64, ACC));
    GateRef imm = CastInt64ToFloat64(Int64Argument(PARAM_INDEX(BaselineFldaiImm64, IMM)));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = DoubleToTaggedDoublePtr(imm);
    Return(*varAcc);
}

void BaselineReturnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef sp = PtrArgument(THIRD_PARAMETER);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    GateRef frame = GetFrame(*varSp);

    auto env = GetEnvironment();
    GateRef pc = GetPcFromFrame(frame);
    GateRef curFunction = GetFunctionFromFrame(frame);
    GateRef curMethod =
        Load(VariableType::JS_ANY(), curFunction, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef constpool =
        Load(VariableType::JS_POINTER(), curMethod, IntPtr(Method::CONSTANT_POOL_OFFSET));
    GateRef profileTypeInfo =
        Load(VariableType::JS_POINTER(), curFunction, IntPtr(JSFunction::PROFILE_TYPE_INFO_OFFSET));
    GateRef hotnessCounter =
        Load(VariableType::INT32(), curMethod, IntPtr(Method::LITERAL_INFO_OFFSET));
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);

    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    ProfileOperation callback;

    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label dispatch(env);
    Label slowPath(env);
    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame,
                 IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc, *varSp, currentSp });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        GateRef function = GetFunctionFromFrame(prevState);
        GateRef method = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                       { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
                         *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

void BaselineLdlexvarImm8Imm8StubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdlexvarImm8Imm8, ACC));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineLdlexvarImm8Imm8, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineLdlexvarImm8Imm8, SLOT));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdlexvarImm8Imm8, SP));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(GetFrame(sp)));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    varAcc = variable;
    Return(variable);
}

void BaselineStlexvarImm8Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStlexvarImm8Imm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStlexvarImm8Imm8, ACC));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineStlexvarImm8Imm8, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineStlexvarImm8Imm8, SLOT));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStlexvarImm8Imm8, SP));

    auto env = GetEnvironment();
    GateRef value = acc;
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(GetFrame(sp)));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    Return();
}

void BaselineMovV4V4StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineMovV4V4, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineMovV4V4, SP));
    GateRef vdst = ZExtInt8ToPtr(Int32Argument((PARAM_INDEX(BaselineMovV4V4, VDST))));
    GateRef vsrc = ZExtInt8ToPtr(Int32Argument((PARAM_INDEX(BaselineMovV4V4, VSRC))));

    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    Return();
}

// GLUE
void BaselineJnstricteqV8Imm16StubBuilder::GenerateCircuit()
{
    Return();
}

void BaselineMovV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineMovV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineMovV8V8, SP));
    GateRef vdst = ZExtInt8ToPtr(Int32Argument(PARAM_INDEX(BaselineMovV8V8, VDST)));
    GateRef vsrc = ZExtInt8ToPtr(Int32Argument(PARAM_INDEX(BaselineMovV8V8, VSRC)));

    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    Return();
}

void BaselineMovV16V16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineMovV16V16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineMovV16V16, SP));
    GateRef vdst = ZExtInt16ToPtr(Int32Argument(PARAM_INDEX(BaselineMovV16V16, VDST)));
    GateRef vsrc = ZExtInt16ToPtr(Int32Argument(PARAM_INDEX(BaselineMovV16V16, VSRC)));

    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    Return();
}

void BaselineAsyncgeneratorrejectV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncgeneratorrejectV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineAsyncgeneratorrejectV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncgeneratorrejectV8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineAsyncgeneratorrejectV8, V0));
    GateRef asyncGenerator = GetVregValue(sp, ZExtInt8ToPtr(v0));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncGeneratorReject),
                              { asyncGenerator, acc });
    Return(res);
}

void BaselineNopStubBuilder::GenerateCircuit()
{
    Return();
}

void BaselineSetgeneratorstateImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSetgeneratorstateImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSetgeneratorstateImm8, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineSetgeneratorstateImm8, INDEX));

    GateRef value = acc;
    CallRuntime(glue, RTSTUB_ID(SetGeneratorState), { value, IntToTaggedInt(index) });
    Return();
}

void BaselineGetasynciteratorImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetasynciteratorImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetasynciteratorImm8, GLUE));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(GetAsyncIterator), { *varAcc });
    Return(res);
}

void BaselineLdPrivatePropertyImm8Imm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, ACC));
    GateRef levelIndex = Int32Argument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, INDEX0));
    GateRef slotIndex = Int32Argument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, INDEX1));
    GateRef lexicalEnv = TaggedPointerArgument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, ENV));

    GateRef res = CallRuntime(glue, RTSTUB_ID(LdPrivateProperty), {lexicalEnv,
        IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), acc});  // acc as obj
    Return(res);
}

void BaselineStPrivatePropertyImm8Imm16Imm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStPrivatePropertyImm8Imm16Imm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStPrivatePropertyImm8Imm16Imm16V8, SP));
    GateRef levelIndex = Int32Argument(PARAM_INDEX(BaselineStPrivatePropertyImm8Imm16Imm16V8, INDEX0));
    GateRef slotIndex = Int32Argument(PARAM_INDEX(BaselineStPrivatePropertyImm8Imm16Imm16V8, INDEX1));
    GateRef index3 = Int32Argument(PARAM_INDEX(BaselineStPrivatePropertyImm8Imm16Imm16V8, INDEX2));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(index3));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef lexicalEnv = GetEnvFromFrame(frame);

    GateRef res = CallRuntime(glue, RTSTUB_ID(StPrivateProperty), {lexicalEnv,
        IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), obj, acc});  // acc as value
    (void)res;
    Return();
}

void BaselineTestInImm8Imm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, ACC));
    GateRef levelIndex = Int32Argument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, INDEX0));
    GateRef slotIndex = Int32Argument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, INDEX1));
    GateRef lexicalEnv = TaggedPointerArgument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, ENV));

    GateRef res = CallRuntime(glue, RTSTUB_ID(TestIn), {lexicalEnv,
        IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), acc});  // acc as obj
    Return(res);
}

void BaselineDeprecatedLdlexenvPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedLdlexenvPrefNone, ACC));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdlexenvPrefNone, SP));
    GateRef state = GetFrame(sp);

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = GetEnvFromFrame(state);
    Return();
}

void BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, SP));
    GateRef numKeys = Int32Argument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, V1));
    GateRef v2 = Int32Argument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, V2));

    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));
    GateRef firstArgRegIdx = ZExtInt8ToInt16(v2);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithExcludedKeys),
        { Int16ToTaggedInt(numKeys), obj, Int16ToTaggedInt(firstArgRegIdx) });
    Return(res);
}

void BaselineThrowPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowPrefNone, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineThrowPrefNone, ACC));

    CallRuntime(glue, RTSTUB_ID(Throw), { acc });
    Return();
}

void BaselineDeprecatedPoplexenvPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedPoplexenvPrefNone, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedPoplexenvPrefNone, SP));

    GateRef state = GetFrame(sp);
    GateRef currentLexEnv = GetEnvFromFrame(state);
    GateRef parentLexEnv = GetParentEnv(currentLexEnv);
    SetEnvToFrame(glue, state, parentLexEnv);
    Return();
}

void BaselineWideNewobjrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, ACC));
    GateRef numArgs = Int32Argument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, NUM_ARGS));
    GateRef idx = Int32Argument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, IDX));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, HOTNESS_COUNTER));
    ProfileOperation callback;
    GateRef firstArgRegIdx = ZExtInt8ToInt16(idx);
    GateRef firstArgOffset = Int16(1);
    GateRef ctor = GetVregValue(sp, ZExtInt16ToPtr(firstArgRegIdx));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef actualNumArgs = ZExtInt16ToInt32(Int16Sub(numArgs, firstArgOffset));

    Label ctorIsHeapObject(env);
    Label ctorIsJSFunction(env);
    Label fastPath(env);
    Label slowPath(env);
    Label checkResult(env);
    Label threadCheck(env);
    Label dispatch(env);
    Label ctorIsBase(env);
    Label ctorNotBase(env);
    Label isException(env);

    Branch(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    Branch(IsJSFunction(ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    Branch(IsConstructor(ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        Branch(IsBase(ctor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            NewObjectStubBuilder newBuilder(this);
            thisObj = newBuilder.FastNewThisObject(glue, ctor);
            Branch(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(
            PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
        GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8));
        res = JSCallDispatchForBaseline(glue, ctor, actualNumArgs, jumpSize, hotnessCounter,
                                        JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV,
                                        { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj }, callback);
        Jump(&threadCheck);
    }
    Bind(&slowPath);
    GateRef firstArgIdx = Int16Add(firstArgRegIdx, firstArgOffset);
    GateRef length = Int16Sub(numArgs, firstArgOffset);
    res = CallRuntime(glue, RTSTUB_ID(NewObjRange),
        { ctor, ctor, Int16ToTaggedInt(firstArgIdx), Int16ToTaggedInt(length) });
    Jump(&checkResult);
    Bind(&checkResult);
    {
        Branch(TaggedIsException(*res), &isException, &dispatch);
    }
    Bind(&threadCheck);
    {
        Branch(HasPendingException(glue), &isException, &dispatch);
    }
    Bind(&isException);
    {
        Return(*res);
    }
    Bind(&dispatch);
    varAcc = *res;
    (void)varAcc;
    Return(*res);
}

void BaselineThrowNotexistsPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowNotexistsPrefNone, GLUE));

    CallRuntime(glue, RTSTUB_ID(ThrowThrowNotExists), {});
    Return();
}

void BaselineDeprecatedGetiteratornextPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedGetiteratornextPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedGetiteratornextPrefV8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedGetiteratornextPrefV8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedGetiteratornextPrefV8V8, V1));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef method = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef result = CallRuntime(glue, RTSTUB_ID(GetIteratorNext), { obj, method });
    Return(result);
}

void BaselineWideNewlexenvPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideNewlexenvPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideNewlexenvPrefImm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideNewlexenvPrefImm16, ACC));
    GateRef numVars = Int32Argument(PARAM_INDEX(BaselineWideNewlexenvPrefImm16, INDEX));
    GateRef state = GetFrame(sp);

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    auto env = GetEnvironment();
    auto parent = GetEnvFromFrame(state);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterNew(env);
    newBuilder.NewLexicalEnv(&result, &afterNew, ZExtInt16ToInt32(numVars), parent);
    Bind(&afterNew);
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*result, &notException);
    Bind(&notException);
    varAcc = *result;
    SetEnvToFrame(glue, state, *result);
    Return(*varAcc);
}

void BaselineThrowPatternnoncoerciblePrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowPatternnoncoerciblePrefNone, GLUE));

    CallRuntime(glue, RTSTUB_ID(ThrowPatternNonCoercible), {});
    Return();
}

void BaselineDeprecatedCreatearraywithbufferPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, SP));
    GateRef immI16 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, IMM_I16));
    GateRef slotIdI8 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, SLOT_ID_I8));
    GateRef profileTypeInfo =
        TaggedPointerArgument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, PROFILE_TYPE_INFO));
    GateRef pc = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, PC));
    GateRef imm = ZExtInt16ToInt32(immI16);
    GateRef slotId = ZExtInt8ToInt32(slotIdI8);

    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    ProfileOperation callback;
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, currentFunc, { pc, 0, true }, profileTypeInfo, slotId, callback);
    Return(res);
}

void BaselineWideNewlexenvwithnamePrefImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, ACC));
    GateRef numVars = Int32Argument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, INDEX0));
    GateRef scopeId = Int32Argument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, INDEX1));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewLexicalEnvWithName),
                              { Int16ToTaggedInt(numVars), Int16ToTaggedInt(scopeId) });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
    Bind(&notException);
    varAcc = res;
    GateRef state = GetFrame(sp);
    SetEnvToFrame(glue, state, res);
    Return(*varAcc);
}

void BaselineThrowDeletesuperpropertyPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowDeletesuperpropertyPrefNone, GLUE));

    CallRuntime(glue, RTSTUB_ID(ThrowDeleteSuperProperty), {});
    Return();
}

void BaselineDeprecatedCreateobjectwithbufferPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreateobjectwithbufferPrefImm16, GLUE));
    GateRef immI16 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCreateobjectwithbufferPrefImm16, IMM_I16));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreateobjectwithbufferPrefImm16, SP));
    GateRef imm = ZExtInt16ToInt32(immI16);

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef module = GetModuleFromFunction(func);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithBuffer), { result });
    Return(res);
}

// GLUE, SP, V0, V1
void BaselineNewobjrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, V1));

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);
    ProfileOperation callback;

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef numArgs = ZExtInt8ToInt16(v0);
    GateRef firstArgRegIdx = ZExtInt8ToInt16(v1);
    GateRef firstArgOffset = Int16(1);
    GateRef ctor = GetVregValue(sp, ZExtInt16ToPtr(firstArgRegIdx));
    GateRef actualNumArgs = ZExtInt16ToInt32(Int16Sub(numArgs, firstArgOffset));

    Label ctorIsHeapObject(env);
    Label ctorIsJSFunction(env);
    Label fastPath(env);
    Label slowPath(env);
    Label checkResult(env);
    Label threadCheck(env);
    Label dispatch(env);
    Label ctorIsBase(env);
    Label ctorNotBase(env);
    Label isException(env);

    Branch(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    Branch(IsJSFunction(ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    Branch(IsConstructor(ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        Branch(IsBase(ctor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            NewObjectStubBuilder newBuilder(this);
            thisObj = newBuilder.FastNewThisObject(glue, ctor);
            Branch(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(
            PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
        GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::IMM8_IMM8_V8));
        METHOD_ENTRY_ENV_DEFINED(ctor);
        res = JSCallDispatchForBaseline(glue, ctor, actualNumArgs, jumpSize, hotnessCounter,
                                        JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV,
                                        { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj }, callback);
        Jump(&threadCheck);
    }
    Bind(&slowPath);
    GateRef firstArgIdx = Int16Add(firstArgRegIdx, firstArgOffset);
    GateRef length = Int16Sub(numArgs, firstArgOffset);
    res = CallRuntime(glue, RTSTUB_ID(NewObjRange),
        { ctor, ctor, Int16ToTaggedInt(firstArgIdx), Int16ToTaggedInt(length) });
    Jump(&checkResult);
    Bind(&checkResult);
    {
        Branch(TaggedIsException(*res), &isException, &dispatch);
    }
    Bind(&threadCheck);
    {
        Branch(HasPendingException(glue), &isException, &dispatch);
    }
    Bind(&isException);
    {
        Return(*res);
    }
    Bind(&dispatch);
    Return(*res);
}

// GLUE, SP, V0, V1
void BaselineNewobjrangeImm16Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, V1));

    GateRef frame = GetFrame(sp);
    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(frame));
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);
    ProfileOperation callback;

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef numArgs = ZExtInt8ToInt16(v0);
    GateRef firstArgRegIdx = ZExtInt8ToInt16(v1);
    GateRef firstArgOffset = Int16(1);
    GateRef ctor = GetVregValue(sp, ZExtInt16ToPtr(firstArgRegIdx));
    GateRef actualNumArgs = ZExtInt16ToInt32(Int16Sub(numArgs, firstArgOffset));

    Label ctorIsHeapObject(env);
    Label ctorIsJSFunction(env);
    Label fastPath(env);
    Label slowPath(env);
    Label checkResult(env);
    Label threadCheck(env);
    Label dispatch(env);
    Label ctorIsBase(env);
    Label ctorNotBase(env);
    Label isException(env);

    Branch(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    Branch(IsJSFunction(ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    Branch(IsConstructor(ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        Branch(IsBase(ctor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            NewObjectStubBuilder newBuilder(this);
            thisObj = newBuilder.FastNewThisObject(glue, ctor);
            Branch(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(
                PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
        GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16_IMM8_V8));
        METHOD_ENTRY_ENV_DEFINED(ctor);
        res = JSCallDispatchForBaseline(glue, ctor, actualNumArgs, jumpSize, hotnessCounter,
                                        JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV,
                                        { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj }, callback);
        Jump(&threadCheck);
    }
    Bind(&slowPath);
    GateRef firstArgIdx = Int16Add(firstArgRegIdx, firstArgOffset);
    GateRef length = Int16Sub(numArgs, firstArgOffset);
    res = CallRuntime(glue, RTSTUB_ID(NewObjRange),
                      { ctor, ctor, Int16ToTaggedInt(firstArgIdx), Int16ToTaggedInt(length) });
    Jump(&checkResult);
    Bind(&checkResult);
    {
        Branch(TaggedIsException(*res), &isException, &dispatch);
    }
    Bind(&threadCheck);
    {
        Branch(HasPendingException(glue), &isException, &dispatch);
    }
    Bind(&isException);
    {
        Return(*res);
    }
    Bind(&dispatch);
    Return(*res);
}

// GLUE, SP, ACC, V0, V1, HOTNESS_COUNTER
void BaselineWideCallrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, V1));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, HOTNESS_COUNTER));
    ProfileOperation callback;
    GateRef actualNumArgs = ZExtInt16ToInt32(v0);
    GateRef argv = PtrAdd(sp, PtrMul(ZExtInt8ToPtr(v1), IntPtr(8))); // 8: byteSize

    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(WIDE_CALLRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_WITH_ARGV, { numArgs, argv }, callback);
    Return(res);
}

void BaselineThrowConstassignmentPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowConstassignmentPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowConstassignmentPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineThrowConstassignmentPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    CallRuntime(glue, RTSTUB_ID(ThrowConstAssignment), { value });
    Return();
}

void BaselineDeprecatedTonumberPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedTonumberPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedTonumberPrefV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedTonumberPrefV8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedTonumberPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    (void) acc;
    auto env = GetEnvironment();
    Label valueIsNumber(env);
    Label valueNotNumber(env);
    Branch(TaggedIsNumber(value), &valueIsNumber, &valueNotNumber);
    Bind(&valueIsNumber);
    {
        Return(value);
    }
    Bind(&valueNotNumber);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumber), { value });
        Return(res);
    }
}

void BaselineWideCallthisrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, V1));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, HOTNESS_COUNTER));

    GateRef actualNumArgs = ZExtInt16ToInt32(v0);
    GateRef thisReg = ZExtInt8ToPtr(v1);
    GateRef thisValue = GetVregValue(sp, thisReg);
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(thisReg, IntPtr(1)), IntPtr(8))); // 1: skip this
    ProfileOperation callback;

    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(WIDE_CALLTHISRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_THIS_WITH_ARGV, { numArgs, argv, thisValue }, callback);
    Return(res);
}

void BaselineThrowIfnotobjectPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowIfnotobjectPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowIfnotobjectPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineThrowIfnotobjectPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    Label isEcmaObject(env);
    Label notEcmaObject(env);
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(value), &isHeapObject, &notEcmaObject);
    Bind(&isHeapObject);
    Branch(TaggedObjectIsEcmaObject(value), &isEcmaObject, &notEcmaObject);
    Bind(&isEcmaObject);
    {
        Return();
    }
    Bind(&notEcmaObject);
    CallRuntime(glue, RTSTUB_ID(ThrowIfNotObject), {});
    Return();
}

void BaselineDeprecatedTonumericPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedTonumericPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedTonumericPrefV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedTonumericPrefV8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedTonumericPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    Label valueIsNumeric(env);
    Label valueNotNumeric(env);
    Branch(TaggedIsNumeric(value), &valueIsNumeric, &valueNotNumeric);
    Bind(&valueIsNumeric);
    {
        varAcc = value;
        (void)varAcc;
        Return(value);
    }
    Bind(&valueNotNumeric);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumeric), { value });
        Return(res);
    }
}

void BaselineWideSupercallthisrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideSupercallthisrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideSupercallthisrangePrefImm16V8, SP));
    GateRef range = Int32Argument(PARAM_INDEX(BaselineWideSupercallthisrangePrefImm16V8, INDEX0));
    GateRef index1 = Int32Argument(PARAM_INDEX(BaselineWideSupercallthisrangePrefImm16V8, INDEX1));

    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    GateRef v0 = ZExtInt8ToInt16(index1);
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { currentFunc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
    Return(res);
}

void BaselineThrowUndefinedifholePrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowUndefinedifholePrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowUndefinedifholePrefV8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineThrowUndefinedifholePrefV8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineThrowUndefinedifholePrefV8V8, V1));

    GateRef hole = GetVregValue(sp, ZExtInt8ToPtr(v0));
    auto env = GetEnvironment();
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(hole), &isHole, &notHole);
    Bind(&notHole);
    {
        Return();
    }
    Bind(&isHole);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));
    CallRuntime(glue, RTSTUB_ID(ThrowUndefinedIfHole), { obj });
    Return();
}

void BaselineThrowUndefinedifholewithnamePrefId16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowUndefinedifholewithnamePrefId16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineThrowUndefinedifholewithnamePrefId16, ACC));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowUndefinedifholewithnamePrefId16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineThrowUndefinedifholewithnamePrefId16, STRING_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef hole = acc;
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(hole), &isHole, &notHole);
    Bind(&notHole);
    {
        Return();
    }
    Bind(&isHole);
    GateRef str = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    CallRuntime(glue, RTSTUB_ID(ThrowUndefinedIfHole), { str });
    Return();
}

void BaselineDeprecatedNegPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedNegPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedNegPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedNegPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    OperationsStubBuilder builder(this);
    GateRef result = builder.Neg(glue, value);
    Return(result);
}

void BaselineWideSupercallarrowrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, ACC));
    GateRef range = Int32Argument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, RANGE));
    GateRef v0I8 = Int32Argument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, V0_I8));
    GateRef v0 = ZExtInt8ToInt16(v0I8);

    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { acc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
    Return(res);
}

void BaselineThrowIfsupernotcorrectcallPrefImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm8, ACC));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm8, IMM));

    GateRef res = CallRuntime(glue, RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        { Int8ToTaggedInt(imm), acc }); // acc is thisValue
    (void)res;
    Return();
}

void BaselineDeprecatedNotPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedNotPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedNotPrefV8, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineDeprecatedNotPrefV8, INDEX));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(index));

    OperationsStubBuilder builder(this);
    GateRef result = builder.Not(glue, value);
    Return(result);
}

void BaselineWideLdobjbyindexPrefImm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideLdobjbyindexPrefImm32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideLdobjbyindexPrefImm32, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideLdobjbyindexPrefImm32, INDEX));

    ProfileOperation callback;
    auto env = GetEnvironment();
    GateRef receiver = acc;
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, receiver, index, callback);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        Return(result);
    }
}

void BaselineThrowIfsupernotcorrectcallPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm16, ACC));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm16, IMM));

    GateRef res = CallRuntime(glue, RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        { Int16ToTaggedInt(imm), acc }); // acc is thisValue
    (void)res;
    Return();
}

void BaselineDeprecatedIncPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedIncPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedIncPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedIncPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    OperationsStubBuilder builder(this);
    GateRef result = builder.Inc(glue, value);
    Return(result);
}

void BaselineWideStobjbyindexPrefV8Imm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, V0));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, INDEX));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = SetPropertyByIndex(glue, receiver, index, acc, false);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return();
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        (void)result;
        Return();
    }
}

void BaselineDeprecatedDecPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedDecPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedDecPrefV8, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineDeprecatedDecPrefV8, INDEX));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(index));

    OperationsStubBuilder builder(this);
    GateRef result = builder.Dec(glue, value);
    Return(result);
}

void BaselineWideStownbyindexPrefV8Imm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, V0));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, INDEX));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    Label notClassConstructor(env);
    Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    Bind(&notClassConstructor);
    Label notClassPrototype(env);
    Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        GateRef result = SetPropertyByIndex(glue, receiver, index, acc, true); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return();
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        (void)result;
        Return();
    }
}

void BaselineDeprecatedCallarg0PrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallarg0PrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallarg0PrefV8, SP));
    GateRef funcReg = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallarg0PrefV8, FUNC_REG));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallarg0PrefV8, HOTNESS_COUNTER));
    ProfileOperation callback;
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));

    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG0);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARG0_PREF_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::DEPRECATED_CALL_ARG0, {}, callback);
    Return(res);
}

void BaselineWideCopyrestargsPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideCopyrestargsPrefImm16, GLUE));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideCopyrestargsPrefImm16, INDEX));
    GateRef restIdx = ZExtInt16ToInt32(index);

    GateRef res = CallRuntime(glue, RTSTUB_ID(CopyRestArgs), { IntToTaggedInt(restIdx) });
    Return(res);
}

void BaselineDeprecatedCallarg1PrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallarg1PrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallarg1PrefV8V8, SP));
    GateRef funcReg = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallarg1PrefV8V8, FUNC_REG));
    GateRef a0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallarg1PrefV8V8, A0));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallarg1PrefV8V8, HOTNESS_COUNTER));
    ProfileOperation callback;
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));

    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG1);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARG1_PREF_V8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::DEPRECATED_CALL_ARG1, { a0Value }, callback);
    Return(res);
}

void BaselineWideLdlexvarPrefImm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideLdlexvarPrefImm16Imm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideLdlexvarPrefImm16Imm16, ACC));
    GateRef levelI16 = Int32Argument(PARAM_INDEX(BaselineWideLdlexvarPrefImm16Imm16, LEVEL_I16));
    GateRef slotI16 = Int32Argument(PARAM_INDEX(BaselineWideLdlexvarPrefImm16Imm16, SLOT_I16));

    GateRef level = ZExtInt16ToInt32(levelI16);
    GateRef slot = ZExtInt16ToInt32(slotI16);
    GateRef state = GetFrame(sp);

    (void) acc;
    auto env = GetEnvironment();
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    Return(variable);
}

void BaselineDeprecatedCallargs2PrefV8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallargs2PrefV8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallargs2PrefV8V8V8, SP));
    GateRef funcReg = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallargs2PrefV8V8V8, FUNC_REG));
    GateRef a0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallargs2PrefV8V8V8, A0));
    GateRef a1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallargs2PrefV8V8V8, A1));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallargs2PrefV8V8V8, HOTNESS_COUNTER));
    ProfileOperation callback;
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(sp, ZExtInt8ToPtr(a1));

    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS2);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARGS2_PREF_V8_V8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::DEPRECATED_CALL_ARG2, { a0Value, a1Value }, callback);
    Return(res);
}

void BaselineWideStlexvarPrefImm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, ACC));
    GateRef levelI16 = Int32Argument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, LEVEL_I16));
    GateRef slotI16 = Int32Argument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, SLOT_I16));

    GateRef level = ZExtInt16ToInt32(levelI16);
    GateRef slot = ZExtInt16ToInt32(slotI16);
    GateRef state = GetFrame(sp);

    auto env = GetEnvironment();
    GateRef value = acc;
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    Return();
}

void BaselineDeprecatedCallargs3PrefV8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallargs3PrefV8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallargs3PrefV8V8V8V8, SP));
    GateRef funcReg = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallargs3PrefV8V8V8V8, FUNC_REG));
    GateRef a0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallargs3PrefV8V8V8V8, A0));
    GateRef a1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallargs3PrefV8V8V8V8, A1));
    GateRef a2 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallargs3PrefV8V8V8V8, A2));
    ProfileOperation callback;
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(sp, ZExtInt8ToPtr(a1));
    GateRef a2Value = GetVregValue(sp, ZExtInt8ToPtr(a2));

    GateRef method = GetMethodFromFunction(GetFunctionFromFrame(GetFrame(sp)));
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);

    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS3);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::DEPRECATED_CALL_ARG3, { a0Value, a1Value, a2Value }, callback);
    Return(res);
}

void BaselineWideGetmodulenamespacePrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideGetmodulenamespacePrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideGetmodulenamespacePrefImm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideGetmodulenamespacePrefImm16, INDEX));

    (void) acc;
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespaceByIndex), { Int16ToTaggedInt(index) });
    Return(moduleRef);
}

void BaselineDeprecatedCallrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, INDEX));
    GateRef funcReg = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, FUNC_REG));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, HOTNESS_COUNTER));
    ProfileOperation callback;
    GateRef actualNumArgs = ZExtInt16ToInt32(index);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(ZExtInt8ToPtr(funcReg), IntPtr(1)), IntPtr(8))); // 1: skip function

    GateRef jumpSize = INT_PTR(DEPRECATED_CALLRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::DEPRECATED_CALL_WITH_ARGV, { numArgs, argv }, callback);
    Return(res);
}

void BaselineWideStmodulevarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStmodulevarPrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideStmodulevarPrefImm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideStmodulevarPrefImm16, INDEX));

    GateRef value = acc;
    CallRuntime(glue, RTSTUB_ID(StModuleVarByIndex), { Int16ToTaggedInt(index), value });
    Return();
}

void BaselineDeprecatedCallspreadPrefV8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallspreadPrefV8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallspreadPrefV8V8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallspreadPrefV8V8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallspreadPrefV8V8V8, V1));
    GateRef v2 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallspreadPrefV8V8V8, V2));

    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));
    GateRef array = GetVregValue(sp, ZExtInt8ToPtr(v2));

    GateRef res = CallRuntime(glue, RTSTUB_ID(CallSpread), { func, obj, array });
    Return(res);
}

void BaselineWideLdlocalmodulevarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideLdlocalmodulevarPrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideLdlocalmodulevarPrefImm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideLdlocalmodulevarPrefImm16, INDEX));

    (void) acc;
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdLocalModuleVarByIndex), { Int16ToTaggedInt(index) });

    Return(moduleRef);
}

void BaselineDeprecatedCallthisrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallthisrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallthisrangePrefImm16V8, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallthisrangePrefImm16V8, INDEX));
    GateRef funcReg = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallthisrangePrefImm16V8, FUNC_REG));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallthisrangePrefImm16V8, HOTNESS_COUNTER));
    ProfileOperation callback;
    funcReg = ZExtInt8ToPtr(funcReg);
    GateRef func = GetVregValue(sp, funcReg);
    GateRef thisValue = GetVregValue(sp, PtrAdd(funcReg, IntPtr(1)));
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(funcReg, IntPtr(2)), IntPtr(8))); // 2: skip function&this

    GateRef actualNumArgs = Int32Sub(ZExtInt16ToInt32(index), Int32(1));  // 1: exclude this
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV, { numArgs, argv, thisValue },
                                            callback);
    Return(res);
}

void BaselineWideLdexternalmodulevarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideLdexternalmodulevarPrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideLdexternalmodulevarPrefImm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideLdexternalmodulevarPrefImm16, INDEX));

    (void) acc;
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdExternalModuleVarByIndex), { Int16ToTaggedInt(index) });
    Return(moduleRef);
}

// GLUE, SP, METHOD_ID, LITERAL_ID, LENGTH, V0, V1
void BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, SP));
    GateRef methodId = Int32Argument(
        PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, METHOD_ID));
    GateRef literalId = Int32Argument(
        PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, LITERAL_ID));
    GateRef length = Int32Argument(
        PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, LENGTH));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, V1));
    // passed by stack
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, GLUE));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef currentFunc = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(currentFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef lexicalEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v1));

    (void) acc;
    auto env = GetEnvironment();
    GateRef module = GetModuleFromFunction(currentFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer),
                              { proto, lexicalEnv, constpool,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId), module,
                                Int16ToTaggedInt(length)});

    Label isException(env);
    Label isNotException(env);
    Branch(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        Return(res);
    }
    Bind(&isNotException);
    Return(res);
}

void BaselineWideLdpatchvarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideLdpatchvarPrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideLdpatchvarPrefImm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideLdpatchvarPrefImm16, INDEX));

    (void) acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdPatchVar), { Int16ToTaggedInt(index) });
    Return(result);
}

void BaselineDeprecatedResumegeneratorPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedResumegeneratorPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedResumegeneratorPrefV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedResumegeneratorPrefV8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedResumegeneratorPrefV8, V0));
    GateRef curFunc = GetFunctionFromFrame(GetFrame(sp));
    (void)glue;
    (void)curFunc;
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    CallNGCRuntime(glue, RTSTUB_ID(StartCallTimer), { glue, curFunc, False() });
#endif

    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    Branch(TaggedIsAsyncGeneratorObject(obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    Bind(&isAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), obj, resumeResultOffset);
        Jump(&dispatch);
    }
    Bind(&notAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), obj, resumeResultOffset);
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Return(*varAcc);
}

void BaselineWideStpatchvarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStpatchvarPrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideStpatchvarPrefImm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideStpatchvarPrefImm16, INDEX));

    GateRef result = CallRuntime(glue, RTSTUB_ID(StPatchVar), { Int16ToTaggedInt(index), acc });
    CHECK_EXCEPTION(result, INT_PTR(WIDE_STPATCHVAR_PREF_IMM16));
}

void BaselineDeprecatedGetresumemodePrefV8StubBuilder::GenerateCircuit()
{
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedGetresumemodePrefV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedGetresumemodePrefV8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedGetresumemodePrefV8, V0));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    Branch(TaggedIsAsyncGeneratorObject(obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    Bind(&isAsyncGeneratorObj);
    {
        varAcc = IntToTaggedPtr(GetResumeModeFromAsyncGeneratorObject(obj));
        Jump(&dispatch);
    }
    Bind(&notAsyncGeneratorObj);
    {
        varAcc = IntToTaggedPtr(GetResumeModeFromGeneratorObject(obj));
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Return(*varAcc);
}

void BaselineDeprecatedGettemplateobjectPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedGettemplateobjectPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedGettemplateobjectPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedGettemplateobjectPrefV8, V0));
    GateRef literal = GetVregValue(sp, ZExtInt8ToPtr(v0));

    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
    Return(result);
}

void BaselineDeprecatedDelobjpropPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedDelobjpropPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedDelobjpropPrefV8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedDelobjpropPrefV8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedDelobjpropPrefV8V8, V1));

    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef result = CallRuntime(glue, RTSTUB_ID(DelObjProp), { obj, prop });
    Return(result);
}

// GLUE, SP, PC, V0, V1
void BaselineDeprecatedSuspendgeneratorPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedSuspendgeneratorPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedSuspendgeneratorPrefV8V8, SP));
    GateRef pc = PtrArgument(PARAM_INDEX(BaselineDeprecatedSuspendgeneratorPrefV8V8, PC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedSuspendgeneratorPrefV8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedSuspendgeneratorPrefV8V8, V1));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);

    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    GateRef genObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));

    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label updateHotness(env);
    Label isStable(env);
    Label tryContinue(env);

    GateRef res = CallRuntime(glue, RTSTUB_ID(SuspendGenerator), { genObj, value });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        Return();
    }
    Bind(&notException);
    varAcc = res;
    Branch(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &isStable);
    Bind(&isStable);
    {
        Branch(ProfilerStubBuilder(env).IsProfileTypeInfoDumped(*varProfileTypeInfo, callback), &tryContinue,
            &updateHotness);
    }
    Bind(&updateHotness);
    {
        GateRef function = GetFunctionFromFrame(frame);
        GateRef thisMethod = Load(VariableType::JS_ANY(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        SetHotnessCounter(glue, thisMethod, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef curFunc = GetFunctionFromFrame(frame);
    CallNGCRuntime(glue, RTSTUB_ID(EndCallTimer), { glue, curFunc });
#endif
    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { currentSp });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        GateRef function = GetFunctionFromFrame(prevState);
        GateRef thisMethod = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(thisMethod);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(thisMethod);
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch), { currentSp, jumpSize });
        Return();
    }
}

void BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8, SP));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8, V0));
    GateRef v1 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8, V1));
    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncFunctionAwaitUncaught), { asyncFuncObj, value });
    Return(result);
}

void BaselineDeprecatedCopydatapropertiesPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCopydatapropertiesPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCopydatapropertiesPrefV8V8, SP));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedCopydatapropertiesPrefV8V8, V0));
    GateRef v1 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedCopydatapropertiesPrefV8V8, V1));
    GateRef dst = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef src = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef result = CallRuntime(glue, RTSTUB_ID(CopyDataProperties), { dst, src });
    Return(result);
}

void BaselineDeprecatedSetobjectwithprotoPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedSetobjectwithprotoPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedSetobjectwithprotoPrefV8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedSetobjectwithprotoPrefV8V8, ACC));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedSetobjectwithprotoPrefV8V8, V0));
    GateRef v1 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedSetobjectwithprotoPrefV8V8, V1));

    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));

    auto env = GetEnvironment();
    (void)env;
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, obj });
    Return(result);
}

void BaselineDeprecatedLdobjbyvaluePrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, ACC));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, V0));
    GateRef v1 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, V1));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));

    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.DeprecatedLoadObjByValue(glue, receiver, propKey);
    Return(result);
}

void BaselineDeprecatedLdsuperbyvaluePrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbyvaluePrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbyvaluePrefV8V8, SP));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdsuperbyvaluePrefV8V8, V0));
    GateRef v1 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdsuperbyvaluePrefV8V8, V1));

    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, propKey }); // sp for thisFunc
    Return(result);
}

void BaselineDeprecatedLdobjbyindexPrefV8Imm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyindexPrefV8Imm32, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyindexPrefV8Imm32, SP));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbyindexPrefV8Imm32, V0));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbyindexPrefV8Imm32, INDEX));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        ProfileOperation callback;
        GateRef result = GetPropertyByIndex(glue, receiver, index, callback);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Return(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        Return(result);
    }
}

void BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8, V1));

    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedTrue() });
    Return(res);
}

void BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8, V1));

    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedFalse() });
    Return(res);
}

void BaselineDeprecatedStlexvarPrefImm4Imm4V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, SP));
    GateRef levelI4 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, LEVEL_I4));
    GateRef slotI4 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, SLOT_I4));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, V0));

    GateRef level = ZExtInt8ToInt32(levelI4);
    GateRef slot = ZExtInt8ToInt32(slotI4);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);

    auto env = GetEnvironment();
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    Return();
}

void BaselineDeprecatedStlexvarPrefImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm8Imm8V8, SP));
    GateRef levelI8 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm8Imm8V8, LEVEL_I8));
    GateRef slotI8 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm8Imm8V8, SLOT_I8));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm8Imm8V8, V0));

    GateRef level = ZExtInt8ToInt32(levelI8);
    GateRef slot = ZExtInt8ToInt32(slotI8);

    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);

    auto env = GetEnvironment();
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    Return();
}

void BaselineDeprecatedStlexvarPrefImm16Imm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm16Imm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm16Imm16V8, SP));
    GateRef levelI16 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm16Imm16V8, LEVEL_I16));
    GateRef slotI16 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm16Imm16V8, SLOT_I16));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm16Imm16V8, V0));

    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);
    GateRef level = ZExtInt16ToInt32(levelI16);
    GateRef slot = ZExtInt16ToInt32(slotI16);
    auto env = GetEnvironment();
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    Return();
}

void BaselineDeprecatedGetmodulenamespacePrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedGetmodulenamespacePrefId32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedGetmodulenamespacePrefId32, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedGetmodulenamespacePrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedGetmodulenamespacePrefId32, SP));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef prop = GetStringFromConstPool(glue, constpool, stringId);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespace), { prop });
    varAcc = moduleRef;
    Return(*varAcc);
}

void BaselineDeprecatedStmodulevarPrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStmodulevarPrefId32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedStmodulevarPrefId32, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedStmodulevarPrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStmodulevarPrefId32, SP));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef prop = GetStringFromConstPool(glue, constpool, stringId);
    GateRef value = acc;

    CallRuntime(glue, RTSTUB_ID(StModuleVar), { prop, value });
    Return();
}

void BaselineDeprecatedLdobjbynamePrefId32V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, V0));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, STRING_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.DeprecatedLoadObjByName(glue, receiver, propKey);
    Return(result);
}

void BaselineDeprecatedLdsuperbynamePrefId32V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, STRING_ID));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, V0));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { receiver, propKey });
    Return(result);
}

void BaselineDeprecatedLdmodulevarPrefId32Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, STRING_ID));
    GateRef flagI8 = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, FLAG_I8));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, SP));
    GateRef flag = ZExtInt8ToInt32(flagI8);

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef key = GetStringFromConstPool(glue, constpool, stringId);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdModuleVar), { key, IntToTaggedInt(flag) });
    varAcc = moduleRef;
    Return(*varAcc);
}

void BaselineDeprecatedStconsttoglobalrecordPrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStconsttoglobalrecordPrefId32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedStconsttoglobalrecordPrefId32, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedStconsttoglobalrecordPrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStconsttoglobalrecordPrefId32, SP));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedTrue() });
    Return(result);
}

void BaselineDeprecatedStlettoglobalrecordPrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlettoglobalrecordPrefId32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedStlettoglobalrecordPrefId32, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlettoglobalrecordPrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlettoglobalrecordPrefId32, SP));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedFalse() });
    Return(result);
}

void BaselineDeprecatedStclasstoglobalrecordPrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStclasstoglobalrecordPrefId32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedStclasstoglobalrecordPrefId32, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedStclasstoglobalrecordPrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStclasstoglobalrecordPrefId32, SP));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedFalse() });
    Return(result);
}

// ACC, SP
void BaselineDeprecatedLdhomeobjectPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedLdhomeobjectPrefNone, ACC));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdhomeobjectPrefNone, SP));

    GateRef state = GetFunctionFromFrame(GetFrame(sp));
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = GetHomeObjectFromJSFunction(state);
    Return(*varAcc);
}

void BaselineDeprecatedCreateobjecthavingmethodPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreateobjecthavingmethodPrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedCreateobjecthavingmethodPrefImm16, ACC));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreateobjecthavingmethodPrefImm16, SP));
    GateRef immI16 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCreateobjecthavingmethodPrefImm16, IMM_I16));
    GateRef imm = ZExtInt16ToInt32(immI16);

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef module = GetModuleFromFunction(func);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectHavingMethod), { result, acc });
    Return(res);
}

void BaselineDeprecatedDynamicimportPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedDynamicimportPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedDynamicimportPrefV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedDynamicimportPrefV8, ACC));
    GateRef specifier = TaggedArgument(PARAM_INDEX(BaselineDeprecatedDynamicimportPrefV8, VREG));

    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(DynamicImport), { specifier, currentFunc });
    Return(res);
}

void BaselineCallRuntimeNotifyConcurrentResultPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeNotifyConcurrentResultPrefNone, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeNotifyConcurrentResultPrefNone, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeNotifyConcurrentResultPrefNone, ACC));

    GateRef funcObj = GetFunctionFromFrame(GetFrame(sp));
    CallRuntime(glue, RTSTUB_ID(NotifyConcurrentResult), {acc, funcObj});
    Return();
}

// GLUE, SP, SLOT_ID_I8, STRING_ID, V0
void BaselineDefineFieldByNameImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, SP));
    GateRef slotIdI8 = Int32Argument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, SLOT_ID_I8));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, STRING_ID));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, V0));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);
    GateRef slotId = ZExtInt8ToInt32(slotIdI8);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);
    Label icPath(env);
    Label slowPath(env);
    Label exit(env);
    // hclass hit -> ic path
    Label tryGetHclass(env);
    Label firstValueHeapObject(env);
    Label hclassNotHit(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &hclassNotHit, &tryGetHclass);
    Bind(&tryGetHclass);
    {
        GateRef firstValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        Branch(TaggedIsHeapObject(firstValue), &firstValueHeapObject, &hclassNotHit);
        Bind(&firstValueHeapObject);
        GateRef hclass = LoadHClass(*holder);
        Branch(Equal(LoadObjectFromWeakRef(firstValue), hclass), &icPath, &hclassNotHit);
    }
    Bind(&hclassNotHit);
    // found entry -> slow path
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hclass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hclass);
        Label findProperty(env);
        Branch(IsSpecialIndexedObj(jsType), &slowPath, &findProperty);
        Bind(&findProperty);
        Label isDicMode(env);
        Label notDicMode(env);
        Branch(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
        Bind(&isDicMode);
        {
            GateRef array = GetPropertiesArray(*holder);
            GateRef entry = FindEntryFromNameDictionary(glue, array, propKey);
            Branch(Int32NotEqual(entry, Int32(-1)), &slowPath, &loopExit);
        }
        Bind(&notDicMode);
        {
            GateRef layOutInfo = GetLayoutFromHClass(hclass);
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
            GateRef entry = FindElementWithCache(glue, layOutInfo, hclass, propKey, propsNum);
            Branch(Int32NotEqual(entry, Int32(-1)), &slowPath, &loopExit);
        }
        Bind(&loopExit);
        {
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            Branch(TaggedIsHeapObject(*holder), &loopEnd, &icPath);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
    }
    Bind(&icPath);
    {
        // IC do the same thing as stobjbyname
        AccessObjectStubBuilder builder(this);
        ProfileOperation callback;
        StringIdInfo stringIdInfo(constpool, stringId);
        result = builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(DefineField), {receiver, propKey, acc});  // acc as value
        Jump(&exit);
    }
    Bind(&exit);
    Return(*result);
}


// GLUE, SP, SLOT_ID_I8, STRING_ID, V0
void BaselineDefinePropertyByNameImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, SP));
    GateRef slotIdI8 = Int32Argument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, SLOT_ID_I8));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, STRING_ID));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, V0));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);
    GateRef slotId = ZExtInt8ToInt32(slotIdI8);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);
    Label icPath(env);
    Label slowPath(env);
    Label exit(env);
    // hclass hit -> ic path
    Label tryGetHclass(env);
    Label firstValueHeapObject(env);
    Label hclassNotHit(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &hclassNotHit, &tryGetHclass);
    Bind(&tryGetHclass);
    {
        GateRef firstValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        Branch(TaggedIsHeapObject(firstValue), &firstValueHeapObject, &hclassNotHit);
        Bind(&firstValueHeapObject);
        GateRef hclass = LoadHClass(*holder);
        Branch(Equal(LoadObjectFromWeakRef(firstValue), hclass), &icPath, &hclassNotHit);
    }
    Bind(&hclassNotHit);
    // found entry -> slow path
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hclass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hclass);
        Label findProperty(env);
        Branch(IsSpecialIndexedObj(jsType), &slowPath, &findProperty);
        Bind(&findProperty);
        Label isDicMode(env);
        Label notDicMode(env);
        Branch(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
        Bind(&isDicMode);
        {
            GateRef array = GetPropertiesArray(*holder);
            GateRef entry = FindEntryFromNameDictionary(glue, array, propKey);
            Branch(Int32NotEqual(entry, Int32(-1)), &slowPath, &loopExit);
        }
        Bind(&notDicMode);
        {
            GateRef layOutInfo = GetLayoutFromHClass(hclass);
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
            GateRef entry = FindElementWithCache(glue, layOutInfo, hclass, propKey, propsNum);
            Branch(Int32NotEqual(entry, Int32(-1)), &slowPath, &loopExit);
        }
        Bind(&loopExit);
        {
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            Branch(TaggedIsHeapObject(*holder), &loopEnd, &icPath);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
    }
    Bind(&icPath);
    {
        // IC do the same thing as stobjbyname
        AccessObjectStubBuilder builder(this);
        ProfileOperation callback;
        StringIdInfo stringIdInfo(constpool, stringId);
        result = builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(DefineField), {receiver, propKey, acc});  // acc as value
        Jump(&exit);
    }
    Bind(&exit);
    Return(*result);
}

void BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8, V1));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v0));

    GateRef res = CallRuntime(glue, RTSTUB_ID(DefineField), {obj, propKey, acc});  // acc as value
    Return(res);
}

void BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8, INDEX));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8, V0));

    GateRef propKey = IntToTaggedInt(index);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef res = CallRuntime(glue, RTSTUB_ID(DefineField), {obj, propKey, acc});  // acc as value
    Return(res);
}

void BaselineCallRuntimeToPropertyKeyPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeToPropertyKeyPrefNone, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeToPropertyKeyPrefNone, ACC));

    GateRef res = CallRuntime(glue, RTSTUB_ID(ToPropertyKey), {acc});
    Return(res);
}

// GLUE, SP, CONST_POOL, CURRENT_FUNC, COUNT, LITERAL_ID
void BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16, SP));
    GateRef count = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16, COUNT));
    GateRef literalId = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16, LITERAL_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef lexicalEnv = GetEnvFromFrame(GetFrame(sp));
    GateRef module = GetModuleFromFunction(func);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreatePrivateProperty), {lexicalEnv,
        IntToTaggedInt(count), constpool, IntToTaggedInt(literalId), module});
    Return(res);

}

// GLUE, SP, ACC, LEVEL_INDEX, SLOT_INDEX, V0
void BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8, ACC));
    GateRef levelIndex =
        Int32Argument(PARAM_INDEX(BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8, LEVEL_INDEX));
    GateRef slotIndex =
        Int32Argument(PARAM_INDEX(BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8, SLOT_INDEX));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8, V0));

    GateRef lexicalEnv = GetEnvFromFrame(GetFrame(sp));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));

    GateRef res = CallRuntime(glue, RTSTUB_ID(DefinePrivateProperty), {lexicalEnv,
        IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), obj, acc});  // acc as value
    Return(res);
}

void BaselineCallRuntimeCallInitPrefImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, V0));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, HOTNESS_COUNTER));
    GateRef thisValue = GetVregValue(sp, ZExtInt8ToPtr(v0));
    ProfileOperation callback;

    // same as callthis0
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG0);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLRUNTIME_CALLINIT_PREF_IMM8_V8);
    GateRef res = JSCallDispatchForBaseline(glue, func, actualNumArgs, jumpSize, hotnessCounter,
                                            JSCallMode::CALL_THIS_ARG0, { thisValue }, callback);
    Return(res);
}

// GLUE, SP, METHOD_ID, LITERAL_ID, LENGTH, V0
void BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8, SP));
    GateRef methodId = Int32Argument(
        PARAM_INDEX(BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8, METHOD_ID));
    GateRef literalId = Int32Argument(
        PARAM_INDEX(BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8, LITERAL_ID));
    GateRef length = Int32Argument(
        PARAM_INDEX(BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8, LENGTH));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8, V0));

    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef frame = GetFrame(sp);
    GateRef currentFunc = GetFunctionFromFrame(frame);
    GateRef acc = GetAccFromFrame(frame);

    auto env = GetEnvironment();
    (void)env;
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef module = GetModuleFromFunction(currentFunc);
    (void)module;
    // remove constpool in args
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateSharedClass),
                              { proto,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId),
                                Int16ToTaggedInt(length), module });

    Label isException(env);
    Label isNotException(env);
    (void)isException;
    (void)isNotException;
    varAcc = res;
    (void)varAcc;
    Return(res);
}

// GLUE, SP, ACC, LEVEL
void BaselineCallRuntimeLdSendableClassPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeLdSendableClassPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeLdSendableClassPrefImm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeLdSendableClassPrefImm16, ACC));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineCallRuntimeLdSendableClassPrefImm16, LEVEL));
    GateRef lexEnv = GetEnvFromFrame(GetFrame(sp));

    (void) acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSendableClass), { lexEnv, Int16ToTaggedInt(level) });
    Return(result);
}

// GLUE, ACC, SP
void BaselineReturnundefinedStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(FIRST_PARAMETER);
    GateRef acc = TaggedArgument(SECOND_PARAMETER);
    GateRef sp = PtrArgument(THIRD_PARAMETER);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    GateRef frame = GetFrame(*varSp);

    auto env = GetEnvironment();
    GateRef pc = GetPcFromFrame(frame);
    GateRef curFunction = GetFunctionFromFrame(frame);
    GateRef curMethod =
        Load(VariableType::JS_ANY(), curFunction, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef constpool =
        Load(VariableType::JS_POINTER(), curMethod, IntPtr(Method::CONSTANT_POOL_OFFSET));
    GateRef profileTypeInfo =
        Load(VariableType::JS_POINTER(), curFunction, IntPtr(JSFunction::PROFILE_TYPE_INFO_OFFSET));
    GateRef hotnessCounter =
        Load(VariableType::INT16(), curMethod, IntPtr(Method::LITERAL_INFO_OFFSET));
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);

    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    ProfileOperation callback;

    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label dispatch(env);
    Label slowPath(env);
    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame,
                 IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    varAcc = Undefined();
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc, *varSp, currentSp });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        GateRef function = GetFunctionFromFrame(prevState);
        GateRef method = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                       { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
                         *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

// interpreter helper handler
void BaselineExceptionHandlerStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineExceptionHandler, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineExceptionHandler, ACC));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineExceptionHandler, SP));
    GateRef profileTypeInfo = TaggedPointerArgument(PARAM_INDEX(BaselineExceptionHandler, PROFILE_TYPE_INFO));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineExceptionHandler, HOTNESS_COUNTER));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), 0);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), 0);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label pcIsInvalid(env);
    Label pcNotInvalid(env);
    GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env->IsArch32Bit()));
    GateRef exception = Load(VariableType::JS_ANY(), glue, exceptionOffset);
    varPc = TaggedCastToIntPtr(CallRuntime(glue, RTSTUB_ID(UpFrame), {}));
    varSp = GetCurrentFrame(glue);
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcIsInvalid, &pcNotInvalid);
    Bind(&pcIsInvalid);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeUncaughtFrameAndReturn), { glue, *varSp, *varAcc });
        Return();
    }
    Bind(&pcNotInvalid);
    {
        varAcc = exception;
        // clear exception
        Store(VariableType::INT64(), glue, glue, exceptionOffset, Hole());
        GateRef function = GetFunctionFromFrame(GetFrame(*varSp));
        GateRef thisMethod = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(thisMethod);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(thisMethod);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeCaughtFrameAndDispatch), {
            glue, *varSp, *varPc, *varConstpool,
            *varProfileTypeInfo, *varAcc, *varHotnessCounter});
        Return();
    }
}

void BaselineUpdateHotnessStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineUpdateHotness, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineUpdateHotness, SP));
    GateRef offset = Int32Argument(PARAM_INDEX(BaselineUpdateHotness, OFFSET));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);

    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    varHotnessCounter = Int32Add(offset, *varHotnessCounter);
    Label slowPath(env);
    Label exitLabel(env);
    BRANCH(Int32LessThan(*varHotnessCounter, Int32(0)), &slowPath, &exitLabel);
    Bind(&slowPath);
    {
        GateRef iVecOffset = IntPtr(JSThread::GlueData::GetInterruptVectorOffset(env->IsArch32Bit()));
        GateRef interruptsFlag = Load(VariableType::INT8(), glue, iVecOffset);
        varHotnessCounter = Int32(EcmaInterpreter::METHOD_HOTNESS_THRESHOLD);
        Label initialized(env);
        Label callRuntime(env);
        BRANCH(BoolOr(TaggedIsUndefined(*varProfileTypeInfo),
                      Int8Equal(interruptsFlag, Int8(VmThreadControl::VM_NEED_SUSPENSION))),
            &callRuntime, &initialized);
        Bind(&callRuntime);
        varProfileTypeInfo = CallRuntime(glue, RTSTUB_ID(UpdateHotnessCounterWithProf), { func });

        Label handleException(env);
        Label noException(env);
        BRANCH(HasPendingException(glue), &handleException, &noException);
        Bind(&handleException);
        {
            Jump(&exitLabel);
        }
        Bind(&noException);
        {
            Jump(&exitLabel);
        }
        Bind(&initialized);
        ProfilerStubBuilder profiler(this);
        profiler.TryJitCompile(glue, { offset, 0, false }, func, profileTypeInfo);
        Jump(&exitLabel);
    }
    Bind(&exitLabel);
    Return();
}
#undef UPDATE_HOTNESS
#undef PARAM_INDEX
}  // namespace panda::ecmascript::kungfu