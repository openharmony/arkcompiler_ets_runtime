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

#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/compiler/baseline/baseline_stubs.h"
#include "ecmascript/compiler/baseline/baseline_stubs-inl.h"
#include "ecmascript/compiler/baseline/baseline_call_signature.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/profiler_stub_builder.h"
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_async_generator_object.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
static constexpr uint32_t ONE_BYTE_SIZE = 8;
static constexpr uint32_t TWO_BYTE_SIZE = 16;
static constexpr uint32_t THREE_BYTE_SIZE = 24;
static constexpr uint32_t ONE_BYTE_ALL_ONE = 0xFF;
static constexpr uint32_t TWO_BYTE_ALL_ONE = 0xFFFF;

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
        BRANCH(BitOr(TaggedIsUndefined(*varProfileTypeInfo),                                                   \
                     Int8Equal(interruptsFlag, Int8(VmThreadControl::VM_NEED_SUSPENSION))),                    \
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
            DISPATCH_LAST();                                                                                   \
            Return();                                                                                          \
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

#define CHECK_EXCEPTION(res)                                                  \
    CheckException(glue, sp, res)

#define CHECK_EXCEPTION_RETURN(res)                                           \
    CheckExceptionReturn(glue, sp, res)

#define CHECK_EXCEPTION_WITH_JUMP(res, jump)                                      \
    CheckExceptionWithJump(glue, sp, res, acc, jump)

#define CHECK_EXCEPTION_WITH_JUMP_RETURN(res, jump)                               \
    CheckExceptionWithJumpAndReturn(glue, sp, res, acc, jump)

#define CHECK_EXCEPTION_WITH_ACC(res)                                             \
    CheckExceptionWithVar(glue, sp, res, acc)

#define CHECK_EXCEPTION_WITH_VARACC(res)                                          \
    CheckExceptionWithVar(glue, sp, res, *varAcc)

#define CHECK_PENDING_EXCEPTION(res)                                         \
    CheckPendingException(glue, sp, res, acc)

#define INT_PTR(opcode)                                                                   \
    IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Opcode::opcode))

#define METHOD_ENTRY(func)                                                                        \
    auto env = GetEnvironment();                                                                  \
    METHOD_ENTRY_ENV_DEFINED(func)

#define METHOD_EXIT()                                                                             \
    auto debugModeOffset = JSThread::GlueData::GetIsDebugModeOffset(env->Is32Bit());              \
    auto tracingOffset = JSThread::GlueData::GetIsTracingOffset(env->Is32Bit());                  \
    Label NeedCallRuntimeTrue(env);                                                               \
    Label NeedCallRuntimeFalse(env);                                                              \
    Branch(LogicOrBuilder(env)                                                                    \
        .Or(Load(VariableType::BOOL(), glue, IntPtr(debugModeOffset)))                            \
        .Or(Load(VariableType::BOOL(), glue, IntPtr(tracingOffset)))                              \
        .Done(), &NeedCallRuntimeTrue, &NeedCallRuntimeFalse);                                    \
    Bind(&NeedCallRuntimeTrue);                                                                   \
    {                                                                                             \
        CallRuntime(glue, RTSTUB_ID(MethodExit), {});                                             \
        Jump(&NeedCallRuntimeFalse);                                                              \
    }                                                                                             \
    Bind(&NeedCallRuntimeFalse)

#define DISPATCH(opcode) DISPATCH_BAK(OFFSET, INT_PTR(opcode))
#define DISPATCH_BAK(TYPE, ...) DISPATCH_##TYPE(__VA_ARGS__)
#define DISPATCH_LAST()                                                                           \
    DispatchLast(glue, sp, acc)                                                                   \

#define DISPATCH_LAST_WITH_ACC()                                                                  \
    DispatchLast(glue, sp, acc)                                                                   \

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


#define DEFINE_PROFILE_CALLBACK(glue, sp, slotId)                                                    \
    GateRef frame = GetFrame(sp);                                                                    \
    GateRef curFunc = GetFunctionFromFrame(frame);                                                   \
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(curFunc);                               \
    ProfileOperation callback(                                                                       \
        [this, glue, curFunc, slotId, profileTypeInfo](const std::initializer_list<GateRef> &values, \
                                                       OperationType type) {                         \
            ProfilerStubBuilder profiler(this);                                                      \
            profiler.PGOProfiler(glue, curFunc, profileTypeInfo, slotId, values, type);              \
        }, nullptr)

#define DEFINE_BY_NAME(newIc)                                                                                        \
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);                                                           \
    Label icPath(env);                                                                                               \
    Label whichPath(env);                                                                                            \
    Label slowPath(env);                                                                                             \
    Label exit(env);                                                                                                 \
    Label isEcmaObj(env);                                                                                            \
    /*hclass hit -> ic path*/                                                                                        \
    Label tryGetHclass(env);                                                                                         \
    Label firstValueHeapObject(env);                                                                                 \
    Label hclassNotHit(env);                                                                                         \
    BRANCH(IsEcmaObject(receiver), &isEcmaObj, &slowPath);                                                           \
    Bind(&isEcmaObj);                                                                                                \
    BRANCH(TaggedIsUndefined(profileTypeInfo), &hclassNotHit, &tryGetHclass);                                        \
    Bind(&tryGetHclass);                                                                                             \
    {                                                                                                                \
        GateRef firstValue = GetValueFromTaggedArray(profileTypeInfo, slotId);                                       \
        BRANCH(TaggedIsHeapObject(firstValue), &firstValueHeapObject, &hclassNotHit);                                \
        Bind(&firstValueHeapObject);                                                                                 \
        GateRef hclass = LoadHClass(*holder);                                                                        \
        BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass), &whichPath, &hclassNotHit);                         \
    }                                                                                                                \
    Bind(&hclassNotHit);                                                                                             \
    /* found entry -> slow path*/                                                                                    \
    Label loopHead(env);                                                                                             \
    Label loopEnd(env);                                                                                              \
    Label loopExit(env);                                                                                             \
    Jump(&loopHead);                                                                                                 \
    LoopBegin(&loopHead);                                                                                            \
    {                                                                                                                \
        GateRef hclass = LoadHClass(*holder);                                                                        \
        GateRef jsType = GetObjectType(hclass);                                                                      \
        Label findProperty(env);                                                                                     \
        BRANCH(IsSpecialIndexedObj(jsType), &slowPath, &findProperty);                                               \
        Bind(&findProperty);                                                                                         \
        Label isDicMode(env);                                                                                        \
        Label notDicMode(env);                                                                                       \
        BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);                                           \
        Bind(&isDicMode);                                                                                            \
        {                                                                                                            \
            GateRef array = GetPropertiesArray(*holder);                                                             \
            GateRef entry = FindEntryFromNameDictionary(glue, array, propKey);                                       \
            BRANCH(Int32NotEqual(entry, Int32(-1)), &slowPath, &loopExit);                                           \
        }                                                                                                            \
        Bind(&notDicMode);                                                                                           \
        {                                                                                                            \
            GateRef layOutInfo = GetLayoutFromHClass(hclass);                                                        \
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);                                                   \
            GateRef entry = FindElementWithCache(glue, layOutInfo, hclass, propKey, propsNum);                       \
            BRANCH(Int32NotEqual(entry, Int32(-1)), &slowPath, &loopExit);                                           \
        }                                                                                                            \
        Bind(&loopExit);                                                                                             \
        {                                                                                                            \
            holder = GetPrototypeFromHClass(LoadHClass(*holder));                                                    \
            BRANCH(TaggedIsHeapObject(*holder), &loopEnd, &whichPath);                                               \
        }                                                                                                            \
        Bind(&loopEnd);                                                                                              \
        LoopEnd(&loopHead, env, glue);                                                                               \
    }                                                                                                                \
    Bind(&whichPath);                                                                                                \
    {                                                                                                                \
        BRANCH(newIc, &icPath, &slowPath);                                                                           \
    }                                                                                                                \
    Bind(&icPath);                                                                                                   \
    {                                                                                                                \
        /* IC do the same thing as stobjbyname */                                                                    \
        AccessObjectStubBuilder builder(this);                                                                       \
        StringIdInfo stringIdInfo(constpool, stringId);                                                              \
        result = builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);    \
        Jump(&exit);                                                                                                 \
    }                                                                                                                \
    Bind(&slowPath);                                                                                                 \
    {                                                                                                                \
        result = DefineField(glue, receiver, propKey, acc);                                                          \
        Jump(&exit);                                                                                                 \
    }                                                                                                                \
    Bind(&exit)

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

void BaselineTryLdGLobalByNameImm8ID16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineTryLdGLobalByNameImm8ID16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, stringId);
    GateRef result = builder.TryLoadGlobalByName(glue, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result);
}

void BaselineCallArg1Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallArg1Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallArg1Imm8V8, SP));
    GateRef arg0No = Int32Argument(PARAM_INDEX(BaselineCallArg1Imm8V8, ARG0));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallArg1Imm8V8, SLOT_ID));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef a0Value = GetVregValue(sp, ZExtInt32ToPtr(arg0No));
    GateRef actualNumArgs = Int32(ActualNumArgsOfCall::CALLARG1);
    GateRef acc = GetAccFromFrame(frame);
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef jumpSize = INT_PTR(CALLARG1_IMM8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_ARG1);
    callArgs.callArgs = { a0Value, 0, 0 };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineStToGlobalRecordImm16ID16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStToGlobalRecordImm16ID16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStToGlobalRecordImm16ID16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStToGlobalRecordImm16ID16, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStToGlobalRecordImm16ID16, STRING_ID));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, acc, TaggedFalse() });
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineLdaStrID16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdaStrID16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdaStrID16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdaStrID16, STRING_ID));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef result = GetStringFromConstPool(glue, constpool, stringId);
    Return(result);
}

void BaselineLdsymbolStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsymbol, GLUE));

    GateRef result = CallRuntime(glue, RTSTUB_ID(GetSymbolFunction), {});

    Return(result);
}

void BaselineLdglobalStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdglobal, GLUE));

    GateRef result = GetGlobalObject(glue);

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
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGetunmappedargs, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetunmappedargs, ACC));

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
        Return(*argumentsObj);
    }

    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(GetUnmapedArgs), {});
        CHECK_EXCEPTION_WITH_ACC(res);
    }
}

void BaselineAsyncfunctionenterStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionenter, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionenter, SP));

    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncFunctionEnter), {});

    CHECK_EXCEPTION_RETURN(result);
}

void BaselineCreateasyncgeneratorobjV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateasyncgeneratorobjV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateasyncgeneratorobjV8, SP));
    GateRef genFunc = TaggedArgument(PARAM_INDEX(BaselineCreateasyncgeneratorobjV8, GEN_FUNC));

    auto env = GetEnvironment();
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateAsyncGeneratorObj), { genFunc });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        GateRef acc = GetAccFromFrame(GetFrame(sp));
        DISPATCH_LAST();
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
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGetpropiterator, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetpropiterator, ACC));

    NewObjectStubBuilder newBuilder(this);
    GateRef result = newBuilder.EnumerateObjectProperties(glue, acc);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineGetiteratorImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetiteratorImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGetiteratorImm8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineGetiteratorImm8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    GateRef res = GetIterator(glue, acc, callback);

    CHECK_PENDING_EXCEPTION(res);
}

void BaselineGetiteratorImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetiteratorImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGetiteratorImm16, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineGetiteratorImm16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    GateRef res = GetIterator(glue, acc, callback);

    CHECK_PENDING_EXCEPTION(res);
}

void BaselineCloseiteratorImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCloseiteratorImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCloseiteratorImm8V8, SP));
    GateRef iter = TaggedArgument(PARAM_INDEX(BaselineCloseiteratorImm8V8, ITER));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CloseIterator), { iter });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineCloseiteratorImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCloseiteratorImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCloseiteratorImm16V8, SP));
    GateRef iter = TaggedArgument(PARAM_INDEX(BaselineCloseiteratorImm16V8, ITER));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CloseIterator), { iter });
    CHECK_EXCEPTION_WITH_ACC(result);
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
    DEFVARIABLE(prevState, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label isBaselineBuiltinFrame(env);
    Label notBaselineBuiltinFrame(env);
    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label pcEqualBaseline(env);
    Label pcNotEqualBaseline(env);
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
        DISPATCH_LAST();
        Return();
    }
    Bind(&notException);
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

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = Load(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = Load(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    GateRef varPc = GetPcFromFrame(*prevState);
    Branch(IntPtrEqual(varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        GateRef result = CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { res, *varSp, currentSp });
        (void) result;
        Return();
    }
    Bind(&pcNotEqualNullptr);
    BRANCH(IntPtrEqual(varPc, IntPtr(BASELINEJIT_PC_FLAG)), &pcEqualBaseline, &pcNotEqualBaseline);
    Bind(&pcEqualBaseline);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { res, *varSp, currentSp });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(*prevState);
        GateRef thisMethod = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(thisMethod);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(thisMethod);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        GateRef result = CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                                        { glue, currentSp, varPc, *varConstpool, *varProfileTypeInfo,
                                        res, *varHotnessCounter, jumpSize });
        (void) result;
        Return();
    }
}

void BaselineCreateemptyobjectStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateemptyobject, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateemptyobject, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreateemptyobject, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    NewObjectStubBuilder newBuilder(this);
    GateRef result = newBuilder.CreateEmptyObject(glue);
    Return(result);
}

void BaselineCreateemptyarrayImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateemptyarrayImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateemptyarrayImm8, SP));
    GateRef traceId = Int32Argument(PARAM_INDEX(BaselineCreateemptyarrayImm8, TRACE_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreateemptyarrayImm8, SLOT_ID));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    NewObjectStubBuilder newBuilder(this);
    GateRef result = newBuilder.CreateEmptyArray(glue, curFunc, { 0, traceId, false },
                                                 profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineCreateemptyarrayImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateemptyarrayImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateemptyarrayImm16, SP));
    GateRef traceId = Int32Argument(PARAM_INDEX(BaselineCreateemptyarrayImm16, TRACE_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreateemptyarrayImm16, SLOTID));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    NewObjectStubBuilder newBuilder(this);
    GateRef result =
        newBuilder.CreateEmptyArray(glue, curFunc, { 0, traceId, false }, profileTypeInfo, slotId, callback);
    Return(result);
}

void BaselineCreategeneratorobjV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreategeneratorobjV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreategeneratorobjV8, SP));
    GateRef genFunc = TaggedArgument(PARAM_INDEX(BaselineCreategeneratorobjV8, GEN_FUNC));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateGeneratorObj), { genFunc });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineCreateiterresultobjV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateiterresultobjV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateiterresultobjV8V8, SP));
    GateRef value = TaggedArgument(PARAM_INDEX(BaselineCreateiterresultobjV8V8, VALUE));
    GateRef flag = TaggedArgument(PARAM_INDEX(BaselineCreateiterresultobjV8V8, FLAG));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateIterResultObj), { value, flag });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineCreateobjectwithexcludedkeysImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, SP));
    GateRef numKeys = Int32Argument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, NUMKEYS));
    GateRef obj = TaggedArgument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, OBJ));
    GateRef firstArgRegIdx =
        Int32Argument(PARAM_INDEX(BaselineCreateobjectwithexcludedkeysImm8V8V8, FIRST_ARG_REG_IDX));

    GateRef acc = GetAccFromFrame(GetFrame(sp));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithExcludedKeys),
        { Int16ToTaggedInt(numKeys), obj, Int16ToTaggedInt(firstArgRegIdx) });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCallthis0Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthis0Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallthis0Imm8V8, SP));
    GateRef thisValueNo = Int32Argument(PARAM_INDEX(BaselineCallthis0Imm8V8, THIS_VALUE_NO));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallthis0Imm8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef thisValue = GetVregValue(sp, ZExtInt32ToPtr(thisValueNo));
    GateRef acc = GetAccFromFrame(frame);
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG0);
    GateRef jumpSize = INT_PTR(CALLTHIS0_IMM8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG0);
    callArgs.callArgsWithThis = { 0, 0, 0, thisValue };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineCreatearraywithbufferImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, SP));
    GateRef traceId = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, TRACE_ID));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, IMM));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm8Id16, SLOTID));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, curFunc, { 0, traceId, false }, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCreatearraywithbufferImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, SP));
    GateRef traceId = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, TRACE_ID));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, IMM));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreatearraywithbufferImm16Id16, SLOTID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, curFunc, { 0, traceId, false }, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCallthis1Imm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, SP));
    GateRef thisValueId = Int32Argument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, THIS_VALUE_ID));
    GateRef a0ValueId = Int32Argument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, A0_VALUE_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallthis1Imm8V8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef thisValue = GetVregValue(sp, ZExtInt32ToPtr(thisValueId));
    GateRef a0Value = GetVregValue(sp, ZExtInt32ToPtr(a0ValueId));
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef acc = GetAccFromFrame(frame);
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG1);
    GateRef jumpSize = INT_PTR(CALLTHIS1_IMM8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG1);
    callArgs.callArgsWithThis = { a0Value, 0, 0, thisValue };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineCallthis2Imm8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, SP));
    GateRef thisValueId = Int32Argument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, THIS_VALUE_ID));
    GateRef a0ValueId = Int32Argument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, A0_VALUE_ID));
    GateRef a1ValueId = Int32Argument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, A1_VALUE_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallthis2Imm8V8V8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef a0Value = GetVregValue(sp, ZExtInt32ToPtr(a0ValueId));
    GateRef a1Value = GetVregValue(sp, ZExtInt32ToPtr(a1ValueId));
    GateRef thisValue = GetVregValue(sp, ZExtInt32ToPtr(thisValueId));
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef acc = GetAccFromFrame(frame);
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS2);
    GateRef jumpSize = INT_PTR(CALLTHIS2_IMM8_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG2);
    callArgs.callArgsWithThis = { a0Value, a1Value, 0, thisValue };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineCreateobjectwithbufferImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithbufferImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithbufferImm8Id16, SP));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineCreateobjectwithbufferImm8Id16, IMM));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreateobjectwithbufferImm8Id16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef currentEnv = GetEnvFromFrame(frame);
    GateRef module = GetModuleFromFunction(curFunc);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectHavingMethod), { result, currentEnv });
    callback.ProfileCreateObject(res);
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCreateobjectwithbufferImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithbufferImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateobjectwithbufferImm16Id16, SP));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineCreateobjectwithbufferImm16Id16, IMM));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCreateobjectwithbufferImm16Id16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef currentEnv = GetEnvFromFrame(GetFrame(sp));
    GateRef module = GetModuleFromFunction(curFunc);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectHavingMethod), { result, currentEnv });
    callback.ProfileCreateObject(res);
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCreateregexpwithliteralImm8Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm8Id16Imm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm8Id16Imm8, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineCreateregexpwithliteralImm8Id16Imm8, STRING_ID));
    GateRef flags = Int32Argument(PARAM_INDEX(BaselineCreateregexpwithliteralImm8Id16Imm8, FLAGS));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef pattern = GetStringFromConstPool(glue, constpool, stringId);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateRegExpWithLiteral),
                              { pattern, Int8ToTaggedInt(flags) });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCreateregexpwithliteralImm16Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm16Id16Imm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCreateregexpwithliteralImm16Id16Imm8, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineCreateregexpwithliteralImm16Id16Imm8, STRING_ID));
    GateRef flags = Int32Argument(PARAM_INDEX(BaselineCreateregexpwithliteralImm16Id16Imm8, FLAGS));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef pattern = GetStringFromConstPool(glue, constpool, stringId);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateRegExpWithLiteral),
                              { pattern, Int8ToTaggedInt(flags) });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineNewobjapplyImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewobjapplyImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewobjapplyImm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineNewobjapplyImm8V8, ACC));
    GateRef func = TaggedArgument(PARAM_INDEX(BaselineNewobjapplyImm8V8, FUNC));

    GateRef result = CallRuntime(glue, RTSTUB_ID(NewObjApply), { func, acc }); // acc is array
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineNewobjapplyImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewobjapplyImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewobjapplyImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineNewobjapplyImm16V8, ACC));
    GateRef func = TaggedArgument(PARAM_INDEX(BaselineNewobjapplyImm16V8, FUNC));

    GateRef result = CallRuntime(glue, RTSTUB_ID(NewObjApply), { func, acc }); // acc is array
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineNewlexenvImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewlexenvImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineNewlexenvImm8, ACC));
    GateRef numVars = Int32Argument(PARAM_INDEX(BaselineNewlexenvImm8, NUM_VARS));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewlexenvImm8, SP));
    auto parent = GetEnvFromFrame(GetFrame(sp));

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    auto env = GetEnvironment();

    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterNew(env);
    newBuilder.NewLexicalEnv(&result, &afterNew, numVars, parent);
    Bind(&afterNew);
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(*result, &notException);
    Bind(&notException);
    SetEnvToFrame(glue, GetFrame(sp), *result);
    Return(*result);
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
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(res, &notException);
    Bind(&notException);
    GateRef state = GetFrame(sp);
    SetEnvToFrame(glue, state, res);
    Return(res);
}

void BaselineAdd2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineAdd2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Add(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineSub2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineSub2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Sub(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineMul2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineMul2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Mul(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineDiv2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineDiv2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Div(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineMod2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineMod2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Mod(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineEqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineEqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Equal(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineNoteqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineNoteqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.NotEqual(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineLessImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineLessImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Less(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineLesseqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineLesseqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.LessEq(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineGreaterImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineGreaterImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Greater(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineGreatereqImm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineGreatereqImm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.GreaterEq(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineShl2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineShl2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Shl(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineShr2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineShr2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Shr(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineAshr2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineAshr2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Ashr(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineAnd2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineAnd2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.And(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineOr2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineOr2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Or(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineXor2Imm8V8StubBuilder::GenerateCircuit()
{
    DEFINE_BINARYOP_PARAM_AND_PROFILE_CALLBACK(BaselineXor2Imm8V8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Xor(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineExpImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineExpImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineExpImm8V8, SP));
    GateRef base = TaggedArgument(PARAM_INDEX(BaselineExpImm8V8, BASE));
    GateRef acc = GetAccFromFrame(GetFrame(sp));

    GateRef result = CallRuntime(glue, RTSTUB_ID(Exp), { base, acc });
    CHECK_EXCEPTION_WITH_ACC(result);
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
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTonumberImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTonumberImm8, ACC));

    auto env = GetEnvironment();
    Label valueIsNumber(env);
    Label valueNotNumber(env);
    Branch(TaggedIsNumber(acc), &valueIsNumber, &valueNotNumber);
    Bind(&valueIsNumber);
    {
        Return(acc);
    }
    Bind(&valueNotNumber);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(ToNumber), { acc });
        CHECK_EXCEPTION_RETURN(result);
    }
}

void BaselineTonumericImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTonumericImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTonumericImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTonumericImm8, ACC));

    auto env = GetEnvironment();
    Label valueIsNumeric(env);
    Label valueNotNumeric(env);
    Branch(TaggedIsNumeric(acc), &valueIsNumeric, &valueNotNumeric);
    Bind(&valueIsNumeric);
    {
        Return(acc);
    }
    Bind(&valueNotNumeric);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(ToNumeric), { acc });
        CHECK_EXCEPTION_RETURN(result);
    }
}

void BaselineNegImm8StubBuilder::GenerateCircuit()
{
    DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineNegImm8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Neg(glue, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineNotImm8StubBuilder::GenerateCircuit()
{
    DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineNotImm8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Not(glue, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineIncImm8StubBuilder::GenerateCircuit()
{
    DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineIncImm8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Inc(glue, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineDecImm8StubBuilder::GenerateCircuit()
{
    DEFINE_SINGLEOP_PARAM_AND_PROFILE_CALLBACK(BaselineDecImm8);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Dec(glue, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineIsinImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineIsinImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineIsinImm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineIsinImm8V8, ACC));
    GateRef prop = TaggedArgument(PARAM_INDEX(BaselineIsinImm8V8, PROP));
    ProfileOperation callback;

    GateRef result = CallRuntime(glue, RTSTUB_ID(IsIn), { prop, acc }); // acc is obj
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineInstanceofImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineInstanceofImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineInstanceofImm8V8, SP));
    GateRef objId = Int32Argument(PARAM_INDEX(BaselineInstanceofImm8V8, OBJ_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineInstanceofImm8V8, SLOTID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef obj = GetVregValue(sp, ZExtInt32ToPtr(objId));
    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef result = InstanceOf(glue, obj, acc, profileTypeInfo, slotId, callback);
    CHECK_PENDING_EXCEPTION(result);
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

    GateRef result = FastToBooleanBaseline(acc, true);
    Return(result);
}

void BaselineCallRuntimeIstruePrefImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeIstruePrefImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeIstruePrefImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeIstruePrefImm8, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallRuntimeIstruePrefImm8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef result = FastToBooleanWithProfileBaseline(acc, callback, true);
    Return(result);
}

void BaselineIsfalseStubBuilder::GenerateCircuit()
{
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineIsfalse, ACC));

    GateRef result = FastToBooleanBaseline(acc, false);
    Return(result);
}

void BaselineCallRuntimeIsfalsePrefImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeIsfalsePrefImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeIsfalsePrefImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeIsfalsePrefImm8, ACC));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallRuntimeIsfalsePrefImm8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef result = FastToBooleanWithProfileBaseline(acc, callback, false);
    Return(result);
}


void BaselineCallthis3Imm8V8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, SP));
    GateRef thisValueId = Int32Argument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, THIS_VALUE_ID));
    GateRef argIds = Int32Argument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, ARG_IDS));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallthis3Imm8V8V8V8V8, SLOT_ID));

    GateRef arg0Id = Int32And(argIds, Int32(ONE_BYTE_ALL_ONE));
    GateRef arg1Id = Int32And(Int32LSR(argIds, Int32(ONE_BYTE_SIZE)), Int32(ONE_BYTE_ALL_ONE));
    GateRef arg2Id = Int32And(Int32LSR(argIds, Int32(TWO_BYTE_SIZE)), Int32(ONE_BYTE_ALL_ONE));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS3);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef acc = GetAccFromFrame(frame);
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);
    METHOD_ENTRY(acc);
    GateRef a0Value = GetVregValue(sp, ZExtInt32ToPtr(arg0Id));
    GateRef a1Value = GetVregValue(sp, ZExtInt32ToPtr(arg1Id));
    GateRef a2Value = GetVregValue(sp, ZExtInt32ToPtr(arg2Id));
    GateRef thisValue = GetVregValue(sp, ZExtInt32ToPtr(thisValueId));
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef jumpSize = INT_PTR(CALLTHIS3_IMM8_V8_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3);
    callArgs.callArgsWithThis = { a0Value, a1Value, a2Value, thisValue };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineCallthisrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, SP));
    GateRef actualNumArgs = Int32Argument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, ACTUAL_NUM_ARGS));
    GateRef thisReg = Int32Argument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, THIS_REG));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallthisrangeImm8Imm8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef acc = GetAccFromFrame(frame);
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef thisValue = GetVregValue(sp, ZExtInt8ToPtr(thisReg));
    GateRef argv = PtrAdd(sp, PtrMul(PtrAdd(ZExtInt8ToPtr(thisReg), IntPtr(1)), IntPtr(8)));
    GateRef jumpSize = INT_PTR(CALLTHISRANGE_IMM8_IMM8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_WITH_ARGV);
    callArgs.callArgvWithThis = { numArgs, argv, thisValue };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineSupercallthisrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, SP));
    GateRef range = Int32Argument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, RANGE));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, V0));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineSupercallthisrangeImm8Imm8V8, HOTNESS_COUNTER));
    ProfileOperation callback;

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef actualNumArgs = ZExtInt16ToInt32(range);
    GateRef frame = GetFrame(sp);
    GateRef thisFunc = GetFunctionFromFrame(frame);
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
    Label noNeedCheckException(env);
    Label exit(env);

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
        {
            // skip function
            GateRef argv = PtrAdd(sp, PtrMul(ZExtInt16ToPtr(v0), IntPtr(JSTaggedValue::TaggedTypeSize())));
            GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::IMM8_IMM8_V8));
            METHOD_ENTRY_ENV_DEFINED(superCtor);
            JSCallArgs callArgs(JSCallMode::SUPER_CALL_WITH_ARGV);
            callArgs.superCallArgs = {
                thisFunc, Int16ToTaggedInt(v0), ZExtInt32ToPtr(actualNumArgs), argv, *thisObj, newTarget
            };
            CallStubBuilder callBuilder(this, glue, superCtor, actualNumArgs, jumpSize, &res, hotnessCounter, callArgs,
                callback);
            callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
            Bind(&exit);
            Jump(&threadCheck);
        }
    }
    Bind(&slowPath);
    {
        res = CallRuntime(glue, RTSTUB_ID(SuperCall),
                          { thisFunc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
        Jump(&checkResult);
    }
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
        GateRef acc = GetAccFromFrame(frame);
        DISPATCH_LAST();
        Return(acc);
    }
    Bind(&dispatch);
    Return(*res);
    Bind(&noNeedCheckException);
    Return(*res);
}

void BaselineSupercallarrowrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, ACC));
    GateRef range = Int32Argument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, RANGE));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineSupercallarrowrangeImm8Imm8V8, V0));

    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { acc, Int16ToTaggedInt(v0), Int8ToTaggedInt(range) });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineDefinefuncImm8Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, SP));
    GateRef methodId = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, METHODID));
    GateRef length = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, LENGTH));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm8Id16Imm8, SLOT_ID));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);

    auto env = GetEnvironment();
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = DefineFunc(glue, constpool, methodId);
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(result, &notException);
    Bind(&notException);
    {
        SetLengthToFunction(glue, result, length);
        GateRef envHandle = GetEnvFromFrame(frame);
        SetLexicalEnvToFunction(glue, result, envHandle);
        GateRef currentFunc = GetFunctionFromFrame(frame);
        SetModuleToFunction(glue, result, GetModuleFromFunction(currentFunc));
        SetHomeObjectToFunction(glue, result, GetHomeObjectFromFunction(currentFunc));
        callback.ProfileDefineClass(result);
        Return(result);
    }
}

void BaselineDefinefuncImm16Id16Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, SP));
    GateRef methodId = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, METHODID));
    GateRef length = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, LENGTH));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineDefinefuncImm16Id16Imm8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef acc = GetAccFromFrame(frame);

    GateRef result = DefineFunc(glue, constpool, methodId);
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(result, &notException);
    Bind(&notException);
    {
        SetLengthToFunction(glue, result, length);
        GateRef envHandle = GetEnvFromFrame(frame);
        SetLexicalEnvToFunction(glue, result, envHandle);
        GateRef currentFunc = GetFunctionFromFrame(frame);
        SetModuleToFunction(glue, result, GetModuleFromFunction(currentFunc));
        SetHomeObjectToFunction(glue, result, GetHomeObjectFromFunction(currentFunc));
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
        GetMethodFromConstPool(glue, constpool, methodId));
    result = CallRuntime(glue, RTSTUB_ID(DefineMethod), { *result, acc, Int8ToTaggedInt(length),
                                                          lexEnv, GetModule(sp) });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(*result, &notException);
    Bind(&notException);
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
        GetMethodFromConstPool(glue, constpool, methodId));
    result = CallRuntime(glue, RTSTUB_ID(DefineMethod), { *result, acc, Int8ToTaggedInt(length),
                                                          lexEnv, GetModule(sp) });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(*result, &notException);
    Bind(&notException);
    Return(*result);
}

void BaselineCallarg0Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallarg0Imm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallarg0Imm8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallarg0Imm8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef acc = GetAccFromFrame(frame);
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG0);
    GateRef jumpSize = INT_PTR(CALLARG0_IMM8);
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);
    JSCallArgs callArgs(JSCallMode::CALL_ARG0);
    callArgs.callArgs = { 0, 0, 0 };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineCallRuntimeSupercallforwardallargsPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeSupercallforwardallargsPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeSupercallforwardallargsPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineCallRuntimeSupercallforwardallargsPrefV8, V0));
    GateRef thisFunc = GetVregValue(sp, ZExtInt32ToPtr(v0));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCallForwardAllArgs), { thisFunc });
    CHECK_PENDING_EXCEPTION(res);  // CheckPendingException(glue, sp, res, acc)
}

void BaselineSupercallspreadImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSupercallspreadImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineSupercallspreadImm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSupercallspreadImm8V8, ACC));
    GateRef array = TaggedArgument(PARAM_INDEX(BaselineSupercallspreadImm8V8, ARRARY));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineSupercallspreadImm8V8, HOTNESS_COUNTER));
    ProfileOperation callback;

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef newTarget = GetNewTarget(sp);
    GateRef superCtor = GetPrototype(glue, acc);

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
    Label noNeedCheckException(env);
    Label exit(env);

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
        {
            NewObjectStubBuilder objBuilder(this);
            thisObj = objBuilder.FastSuperAllocateThis(glue, superCtor, newTarget);
            Branch(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        {
            GateRef argvLen = Load(VariableType::INT32(), array, IntPtr(JSArray::LENGTH_OFFSET));
            GateRef srcElements = GetCallSpreadArgs(glue, array, callback);
            GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::IMM8_V8));
            METHOD_ENTRY_ENV_DEFINED(superCtor);
            GateRef elementsPtr = PtrAdd(srcElements, IntPtr(TaggedArray::DATA_OFFSET));
            JSCallArgs callArgs(JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV);
            callArgs.superCallArgs = { acc, array, ZExtInt32ToPtr(argvLen), elementsPtr, *thisObj, newTarget };
            CallStubBuilder callBuilder(this, glue, superCtor, argvLen, jumpSize, &res, hotnessCounter, callArgs,
                callback);
            callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
            Bind(&exit);
            Jump(&threadCheck);
        }
    }
    Bind(&slowPath);
    {
        res = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), { acc, array });
        Jump(&threadCheck);
    }
    Bind(&threadCheck);
    {
        GateRef resVal = *res;
        GateRef isError = LogicAndBuilder(env).And(TaggedIsException(resVal)).And(HasPendingException(glue)).Done();
        Branch(isError, &isException, &dispatch);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
        Return(acc);
    }
    Bind(&dispatch);
    Return(*res);
    Bind(&noNeedCheckException);
    Return(*res);
}

void BaselineApplyImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineApplyImm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineApplyImm8V8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineApplyImm8V8V8, ACC));
    GateRef obj = TaggedArgument(PARAM_INDEX(BaselineApplyImm8V8V8, OBJ));
    GateRef array = TaggedArgument(PARAM_INDEX(BaselineApplyImm8V8V8, ARRARY));

    GateRef res = CallRuntime(glue, RTSTUB_ID(CallSpread), { acc, obj, array });
    CHECK_PENDING_EXCEPTION(res);
}

void BaselineCallargs2Imm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, SP));
    GateRef arg0No = Int32Argument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, ARG0_NO));
    GateRef arg1No = Int32Argument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, ARG1_NO));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallargs2Imm8V8V8, SLOT_ID));
    GateRef a0Value = GetVregValue(sp, ZExtInt32ToPtr(arg0No));
    GateRef a1Value = GetVregValue(sp, ZExtInt32ToPtr(arg1No));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS2);
    GateRef jumpSize = INT_PTR(CALLARGS2_IMM8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_ARG2);
    callArgs.callArgs = { a0Value, a1Value, 0 };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineCallargs3Imm8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, SP));
    GateRef arg0No = Int32Argument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, ARG0_NO));
    GateRef arg1No = Int32Argument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, ARG1_NO));
    GateRef arg2No = Int32Argument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, ARG2_NO));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallargs3Imm8V8V8V8, SLOT_ID));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef curMethod = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);
    GateRef arg0Value = GetVregValue(sp, ZExtInt32ToPtr(arg0No));
    GateRef arg1Value = GetVregValue(sp, ZExtInt32ToPtr(arg1No));
    GateRef arg2Value = GetVregValue(sp, ZExtInt32ToPtr(arg2No));
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef acc = GetAccFromFrame(frame);
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS3);
    GateRef jumpSize = INT_PTR(CALLARGS3_IMM8_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_ARG3);
    callArgs.callArgs = { arg0Value, arg1Value, arg2Value };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineCallrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, SP));
    GateRef actualNumArgs = Int32Argument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, ACTUAL_NUM_ARGS));
    GateRef argStart = Int32Argument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, ARG_START));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallrangeImm8Imm8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef acc = GetAccFromFrame(frame);
    GateRef curMethod = GetMethodFromJSFunctionOrProxy(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(curMethod);
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef argv = PtrAdd(sp, PtrMul(ZExtInt32ToPtr(argStart), IntPtr(8)));
    GateRef jumpSize = INT_PTR(CALLRANGE_IMM8_IMM8_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::CALL_WITH_ARGV);
    callArgs.callArgv = { numArgs, argv };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineLdexternalmodulevarImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdexternalmodulevarImm8, GLUE));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineLdexternalmodulevarImm8, INDEX));

    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdExternalModuleVarByIndex), { Int8ToTaggedInt(index) });
    Return(moduleRef);
}

void BaselineLdthisbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdthisbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdthisbynameImm8Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdthisbynameImm8Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdthisbynameImm8Id16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetThisFromFrame(frame);

    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, stringIdInfo, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineDefinegettersetterbyvalueV8V8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, SP));
    GateRef offset = Int32Argument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, OFFSET));
    GateRef vregIds = Int32Argument(PARAM_INDEX(BaselineDefinegettersetterbyvalueV8V8V8V8, VREG_IDS));

    GateRef objectVregId = Int32And(vregIds, Int32(ONE_BYTE_ALL_ONE));
    GateRef propkeyVregId = Int32And(Int32LSR(vregIds, Int32(ONE_BYTE_SIZE)), Int32(ONE_BYTE_ALL_ONE));
    GateRef getterVregId = Int32And(Int32LSR(vregIds, Int32(TWO_BYTE_SIZE)), Int32(ONE_BYTE_ALL_ONE));
    GateRef setterVregId = Int32And(Int32LSR(vregIds, Int32(THREE_BYTE_SIZE)), Int32(ONE_BYTE_ALL_ONE));

    GateRef obj = GetVregValue(sp, ZExtInt32ToPtr(objectVregId));
    GateRef prop = GetVregValue(sp, ZExtInt32ToPtr(propkeyVregId));
    GateRef getter = GetVregValue(sp, ZExtInt32ToPtr(getterVregId));
    GateRef setter = GetVregValue(sp, ZExtInt32ToPtr(setterVregId));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);

    GateRef func = GetFunctionFromFrame(frame);
    GateRef offsetPtr = TaggedPtrToTaggedIntPtr(IntPtr(offset));
    GateRef res = CallRuntime(glue, RTSTUB_ID(DefineGetterSetterByValue),
                              { obj, prop, getter, setter, acc, func, offsetPtr }); // acc is flag
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineLdthisbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdthisbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdthisbynameImm16Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdthisbynameImm16Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdthisbynameImm16Id16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetThisFromFrame(frame);

    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, stringIdInfo, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineStthisbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStthisbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStthisbynameImm8Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStthisbynameImm8Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStthisbynameImm8Id16, SLOT_ID));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(frame);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result =
        builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineStthisbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStthisbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStthisbynameImm16Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStthisbynameImm16Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStthisbynameImm16Id16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(frame);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineLdthisbyvalueImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdthisbyvalueImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdthisbyvalueImm8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdthisbyvalueImm8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);

    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(frame);
    GateRef result = builder.LoadObjByValue(glue, receiver, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineLdthisbyvalueImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdthisbyvalueImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdthisbyvalueImm16, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdthisbyvalueImm16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);

    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(frame);
    GateRef result = builder.LoadObjByValue(glue, receiver, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineStthisbyvalueImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, SLOT_ID));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStthisbyvalueImm8V8, PROP_KEY));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(frame);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineStthisbyvalueImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, SLOT_ID));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStthisbyvalueImm16V8, PROP_KEY));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef receiver = GetThisFromFrame(frame);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineDynamicimportStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDynamicimport, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDynamicimport, SP));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef currentFunc = GetFunctionFromFrame(frame);
    GateRef res = CallRuntime(glue, RTSTUB_ID(DynamicImport), { acc, currentFunc });
    CHECK_EXCEPTION_WITH_ACC(res);
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
    GateRef currentFunc = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(currentFunc);
    GateRef constpool = GetConstpoolFromMethod(method);

    auto env = GetEnvironment();
    GateRef lexicalEnv = GetEnvFromFrame(frame);
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
        GateRef acc = GetAccFromFrame(frame);
        DISPATCH_LAST_WITH_ACC();
        Return(acc);
    }
    Bind(&isNotException);
    callback.ProfileDefineClass(res);
    Return(res);
}

void BaselineDefineclasswithbufferImm16Id16Id16Imm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, SP));
    GateRef methodLiteralId =
        Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, METHOD_LITERIAL_ID));
    GateRef lengthAndProtoId =
        Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, COUNT_SUPERCLASS_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef methodId = Int32And(methodLiteralId, Int32(TWO_BYTE_ALL_ONE));
    GateRef literalId = Int32And(Int32LSR(methodLiteralId, Int32(TWO_BYTE_SIZE)), Int32(TWO_BYTE_ALL_ONE));
    GateRef length = Int32And(lengthAndProtoId, Int32(TWO_BYTE_SIZE));
    GateRef protoVregId = Int32And(Int32LSR(lengthAndProtoId, Int32(TWO_BYTE_SIZE)), Int32(TWO_BYTE_ALL_ONE));
    GateRef proto = GetVregValue(sp, ZExtInt32ToPtr(protoVregId));

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef lexicalEnv = GetEnvFromFrame(frame);
    GateRef module = GetModuleFromFunction(curFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer),
                              { proto, lexicalEnv, constpool,
                                IntToTaggedInt(methodId),
                                IntToTaggedInt(literalId), module,
                                IntToTaggedInt(length)});
    Label isException(env);
    Label isNotException(env);
    Branch(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        GateRef acc = GetAccFromFrame(frame);
        DISPATCH_LAST_WITH_ACC();
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

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef frame = GetFrame(sp);
    GateRef curFunc = GetFunctionFromFrame(frame);
    CallNGCRuntime(glue, RTSTUB_ID(StartCallTimer), { glue, curFunc, False() });
#endif
    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    Branch(TaggedIsAsyncGeneratorObject(acc), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    Bind(&isAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), acc, resumeResultOffset);
        Jump(&dispatch);
    }
    Bind(&notAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), acc, resumeResultOffset);
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

    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    Branch(TaggedIsAsyncGeneratorObject(acc), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    Bind(&isAsyncGeneratorObj);
    {
        varAcc = IntToTaggedPtr(GetResumeModeFromAsyncGeneratorObject(acc));
        Jump(&dispatch);
    }
    Bind(&notAsyncGeneratorObj);
    {
        varAcc = IntToTaggedPtr(GetResumeModeFromGeneratorObject(acc));
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Return(*varAcc);
}

void BaselineGettemplateobjectImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGettemplateobjectImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGettemplateobjectImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGettemplateobjectImm8, ACC));

    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { acc });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineGettemplateobjectImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGettemplateobjectImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGettemplateobjectImm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGettemplateobjectImm16, ACC));

    GateRef literal = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineGetnextpropnameV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetnextpropnameV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGetnextpropnameV8, SP));
    GateRef iter = TaggedArgument(PARAM_INDEX(BaselineGetnextpropnameV8, ITER));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = NextInternal(glue, iter);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineSetobjectwithprotoImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm8V8, ACC));
    GateRef proto = TaggedArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm8V8, PROTO));

    auto env = GetEnvironment();

    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, acc });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    Return();
}

void BaselineDelobjpropV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDelobjpropV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDelobjpropV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDelobjpropV8, ACC));
    GateRef obj = TaggedArgument(PARAM_INDEX(BaselineDelobjpropV8, OBJ));

    GateRef result = DeletePropertyOrThrow(glue, obj, acc);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineAsyncfunctionawaituncaughtV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionawaituncaughtV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionawaituncaughtV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionawaituncaughtV8, ACC));
    GateRef asyncFuncObj = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionawaituncaughtV8, ASYNC_FUNC_OBJ));

    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncFunctionAwaitUncaught), { asyncFuncObj, acc });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineCopydatapropertiesV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCopydatapropertiesV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCopydatapropertiesV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCopydatapropertiesV8, ACC));
    GateRef dst = TaggedArgument(PARAM_INDEX(BaselineCopydatapropertiesV8, DST));

    GateRef result = CallRuntime(glue, RTSTUB_ID(CopyDataProperties), { dst, acc });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineStarrayspreadV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStarrayspreadV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStarrayspreadV8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStarrayspreadV8V8, ACC));
    GateRef dst = TaggedArgument(PARAM_INDEX(BaselineStarrayspreadV8V8, DST));
    GateRef index = TaggedArgument(PARAM_INDEX(BaselineStarrayspreadV8V8, INDEX));

    GateRef result = CallRuntime(glue, RTSTUB_ID(StArraySpread), { dst, index, acc }); // acc is res
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineSetobjectwithprotoImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm16V8, ACC));
    GateRef proto = TaggedArgument(PARAM_INDEX(BaselineSetobjectwithprotoImm16V8, PROTO));

    auto env = GetEnvironment();

    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, acc });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    Return();
}

void BaselineLdobjbyvalueImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbyvalueImm8V8, SLOTID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.LoadObjByValue(glue, receiver, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineLdobjbyvalueImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbyvalueImm16V8, SLOTID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.LoadObjByValue(glue, receiver, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineStobjbyvalueImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, SLOTID));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm8V8V8, PROP_KEY));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineStobjbyvalueImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, SLOTID));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStobjbyvalueImm16V8V8, PROP_KEY));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineStownbyvalueImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbyvalueImm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbyvalueImm8V8V8, SP));
    GateRef receiverId = Int32Argument(PARAM_INDEX(BaselineStownbyvalueImm8V8V8, RECEIVER_ID));
    GateRef propKeyId = Int32Argument(PARAM_INDEX(BaselineStownbyvalueImm8V8V8, PROP_KEY_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbyvalueImm8V8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef receiver = GetVregValue(sp, ZExtInt32ToPtr(receiverId));
    GateRef propKey = GetVregValue(sp, ZExtInt32ToPtr(propKeyId));
    GateRef acc = GetAccFromFrame(frame);

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
        CHECK_EXCEPTION(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByValue), { receiver, propKey, acc });
        CHECK_EXCEPTION(result);
    }
}

void BaselineStownbyvalueImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbyvalueImm16V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbyvalueImm16V8V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbyvalueImm16V8V8, RECEIVER));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStownbyvalueImm16V8V8, PROP_KEY));
    GateRef slotId = TaggedArgument(PARAM_INDEX(BaselineStownbyvalueImm16V8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
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
        CHECK_EXCEPTION(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByValue), { receiver, propKey, acc });
        CHECK_EXCEPTION(result);
    }
}

void BaselineLdsuperbyvalueImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm8V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm8V8, RECEIVER));

    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, acc }); // sp for thisFunc
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineLdsuperbyvalueImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm16V8, ACC));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineLdsuperbyvalueImm16V8, RECEIVER));

    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, acc }); // sp for thisFunc
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineStsuperbyvalueImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStsuperbyvalueImm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStsuperbyvalueImm8V8V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm8V8V8, RECEIVER));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm8V8V8, PROP_KEY));
    GateRef acc = GetAccFromFrame(GetFrame(sp));
    ProfileOperation callback;

    // acc is value, sp for thisFunc
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    CHECK_EXCEPTION(result);
}

void BaselineStsuperbyvalueImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStsuperbyvalueImm16V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStsuperbyvalueImm16V8V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm16V8V8, RECEIVER));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStsuperbyvalueImm16V8V8, PROP_KEY));
    GateRef acc = GetAccFromFrame(GetFrame(sp));
    ProfileOperation callback;

    // acc is value, sp for thisFunc
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    CHECK_EXCEPTION(result);
}

void BaselineLdobjbyindexImm8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbyindexImm8Imm16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    auto env = GetEnvironment();
    GateRef acc = GetAccFromFrame(frame);
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(acc), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, acc, index, callback);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION_WITH_ACC(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { acc, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        CHECK_EXCEPTION_WITH_ACC(result);
    }
}

void BaselineLdobjbyindexImm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbyindexImm16Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdobjbyindexImm16Imm16, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineLdobjbyindexImm16Imm16, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbyindexImm16Imm16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    auto env = GetEnvironment();
    GateRef acc = GetAccFromFrame(frame);
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(acc), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, acc, index, callback);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION_WITH_ACC(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { acc, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        CHECK_EXCEPTION_WITH_ACC(result);
    }
}

void BaselineStobjbyindexImm8V8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbyindexImm8V8Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStobjbyindexImm8V8Imm16, SP));
    GateRef receiver = Int32Argument(PARAM_INDEX(BaselineStobjbyindexImm8V8Imm16, RECEIVER));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStobjbyindexImm8V8Imm16, INDEX));
    GateRef acc = GetAccFromFrame(GetFrame(sp));

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
        CHECK_EXCEPTION(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        CHECK_EXCEPTION(result);
    }
}

void BaselineStobjbyindexImm16V8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, RECEIVER));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStobjbyindexImm16V8Imm16, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);
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
        CHECK_EXCEPTION(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        CHECK_EXCEPTION(result);
    }
}

void BaselineStownbyindexImm8V8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, RECEIVER));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbyindexImm8V8Imm16, SLOTID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreOwnByIndex(
        glue, receiver, index, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineStownbyindexImm16V8Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, RECEIVER));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbyindexImm16V8Imm16, SLOTID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreOwnByIndex(glue, receiver, index, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineAsyncfunctionresolveV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionresolveV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionresolveV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionresolveV8, ACC));
    GateRef asyncFuncObj = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionresolveV8, ASYNC_FUNC_OBJ));

    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, acc, TaggedTrue() });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineAsyncfunctionrejectV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionrejectV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineAsyncfunctionrejectV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionrejectV8, ACC));
    GateRef asyncFuncObj = TaggedArgument(PARAM_INDEX(BaselineAsyncfunctionrejectV8, ASYNC_FUNC_OBJ));

    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, acc, TaggedFalse() });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCopyrestargsImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCopyrestargsImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCopyrestargsImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCopyrestargsImm8, ACC));
    GateRef restIdx = Int32Argument(PARAM_INDEX(BaselineCopyrestargsImm8, REST_IDX));

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
        Return(*res);
    }

    Bind(&slowPath);
    {
        GateRef result2 = CallRuntime(glue, RTSTUB_ID(CopyRestArgs), { IntToTaggedInt(restIdx) });
        CHECK_EXCEPTION_WITH_ACC(result2);
    }
}

void BaselineLdlexvarImm4Imm4StubBuilder::GenerateCircuit()
{
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdlexvarImm4Imm4, SP));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineLdlexvarImm4Imm4, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineLdlexvarImm4Imm4, SLOT));

    auto env = GetEnvironment();
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
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, acc);
    Return();
}

void BaselineGetmodulenamespaceImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetmodulenamespaceImm8, GLUE));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineGetmodulenamespaceImm8, INDEX));

    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespaceByIndex), { IntToTaggedInt(index) });
    Return(moduleRef);
}

void BaselineStmodulevarImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStmodulevarImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineStmodulevarImm8, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineStmodulevarImm8, INDEX));

    CallRuntime(glue, RTSTUB_ID(StModuleVarByIndex), { IntToTaggedInt(index), acc });
    Return();
}

void BaselineTryldglobalbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineTryldglobalbynameImm16Id16, STRING_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, stringId);
    GateRef result = builder.TryLoadGlobalByName(glue, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result);
}

void BaselineTrystglobalbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineTrystglobalbynameImm8Id16, SLOTID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, stringId);
    GateRef result =
        builder.TryStoreGlobalByName(glue, 0, info, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineTrystglobalbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineTrystglobalbynameImm16Id16, STRING_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, stringId);
    GateRef result =
        builder.TryStoreGlobalByName(glue, 0, info, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineLdglobalvarImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdglobalvarImm16Id16, STRING_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, stringId);
    GateRef result = builder.LoadGlobalVar(glue, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result);
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

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, stringId);
    GateRef result = builder.StoreGlobalVar(glue, 0, info, acc, profileTypeInfo, slotId);
    CHECK_EXCEPTION(result);
}

void BaselineLdobjbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdobjbynameImm8Id16, STRING_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, stringIdInfo, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineLdobjbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdobjbynameImm16Id16, STRING_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.LoadObjByName(glue, acc, 0, stringIdInfo, profileTypeInfo,
                                           slotId, callback);
    CHECK_EXCEPTION_RETURN(result);
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
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);

    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result =
        builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineStobjbynameImm16Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, SLOTID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, STRING_ID));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStobjbynameImm16Id16V8, RECEIVER));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    AccessObjectStubBuilder builder(this);
    StringIdInfo stringIdInfo(constpool, stringId);
    GateRef result = builder.StoreObjByName(glue, receiver, 0, stringIdInfo, acc, profileTypeInfo,
                                            slotId, callback);
    CHECK_EXCEPTION(result);
}

void BaselineStownbynameImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, SP));
    GateRef receiverId = Int32Argument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, RECEIVER_ID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbynameImm8Id16V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetVregValue(sp, ZExtInt32ToPtr(receiverId));
    GateRef acc = GetAccFromFrame(frame);
    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
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
        CHECK_EXCEPTION(*result);
    }
}

void BaselineStownbynameImm16Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, RECEIVER));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, STRING_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbynameImm16Id16V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
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
        CHECK_EXCEPTION(*result);
    }
}

void BaselineLdsuperbynameImm8Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsuperbynameImm8Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdsuperbynameImm8Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdsuperbynameImm8Id16, STRING_ID));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { acc, propKey });
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineLdsuperbynameImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdsuperbynameImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdsuperbynameImm16Id16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdsuperbynameImm16Id16, STRING_ID));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { acc, propKey });
    CHECK_EXCEPTION_RETURN(result);
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
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    CHECK_EXCEPTION(result);
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
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    CHECK_EXCEPTION(result);
}

void BaselineLdlocalmodulevarImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdlocalmodulevarImm8, GLUE));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineLdlocalmodulevarImm8, INDEX));

    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdLocalModuleVarByIndex), { Int8ToTaggedInt(index) });
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
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, acc, TaggedTrue() });
    CHECK_EXCEPTION(result);
}

void BaselineStownbyvaluewithnamesetImm8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm8V8V8, SP));
    GateRef receiverId = Int32Argument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm8V8V8, RECEIVER_ID));
    GateRef propKeyId = Int32Argument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm8V8V8, PROP_KEY_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm8V8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef receiver = GetVregValue(sp, ZExtInt32ToPtr(receiverId));
    GateRef propKey = GetVregValue(sp, ZExtInt32ToPtr(propKeyId));
    GateRef acc = GetAccFromFrame(frame);

    auto env = GetEnvironment();
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
                    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
                    Bind(&notException);
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    Return();
                }
            }
        }
    }
    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, acc });
        CHECK_EXCEPTION_WITH_JUMP(res, &notException1);
        Bind(&notException1);
        Return();
    }
}

void BaselineStownbyvaluewithnamesetImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm16V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm16V8V8, SP));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm16V8V8, RECEIVER));
    GateRef propKey = TaggedArgument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm16V8V8, PROP_KEY));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbyvaluewithnamesetImm16V8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    auto env = GetEnvironment();
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
                    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
                    Bind(&notException);
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    Return();
                }
            }
        }
    }
    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, acc });
        CHECK_EXCEPTION_WITH_JUMP(res, &notException1);
        Bind(&notException1);
        Return();
    }
}

void BaselineStownbynamewithnamesetImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, STRING_ID));
    GateRef receiverId = Int32Argument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, RECEIVER_ID));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbynamewithnamesetImm8Id16V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetVregValue(sp, ZExtInt32ToPtr(receiverId));
    GateRef acc = GetAccFromFrame(frame);
    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
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
                    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
                    Bind(&notException);
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    Return();
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, acc });
        CHECK_EXCEPTION_WITH_JUMP(res, &notException1);
        Bind(&notException1);
        Return();
    }
}

void BaselineStownbynamewithnamesetImm16Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, STRING_ID));
    GateRef receiver = TaggedArgument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, RECEIVER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineStownbynamewithnamesetImm16Id16V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
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
                    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
                    Bind(&notException);
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    Return();
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, acc });
        CHECK_EXCEPTION_WITH_JUMP(res, &notException1);
        Bind(&notException1);
        Return();
    }
}

void BaselineLdbigintId16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdbigintId16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdbigintId16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineLdbigintId16, STRING_ID));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef numberBigInt = GetStringFromConstPool(glue, constpool, stringId);
    GateRef res = CallRuntime(glue, RTSTUB_ID(LdBigInt), { numberBigInt });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineFldaiImm64StubBuilder::GenerateCircuit()
{
    GateRef imm = CastInt64ToFloat64(Int64Argument(PARAM_INDEX(BaselineFldaiImm64, IMM)));

    GateRef result = DoubleToTaggedDoublePtr(imm);
    Return(result);
}

void BaselineReturnStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineReturn, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineReturn, SP));
    GateRef curPcOffset = Int32Argument(PARAM_INDEX(BaselineReturn, OFFSET));

    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    GateRef frame = GetFrame(*varSp);
    GateRef acc = GetAccFromFrame(frame);

    auto env = GetEnvironment();
    GateRef pc = GetPcFromFrame(frame);
    GateRef curFunction = GetFunctionFromFrame(frame);
    GateRef curMethod =
        Load(VariableType::JS_ANY(), curFunction, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef constpool =
        Load(VariableType::JS_POINTER(), curMethod, IntPtr(Method::CONSTANT_POOL_OFFSET));
    GateRef raw =
        Load(VariableType::JS_POINTER(), curFunction, IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    GateRef profileTypeInfo =
        Load(VariableType::JS_POINTER(), raw, IntPtr(ProfileTypeInfoCell::VALUE_OFFSET));
    GateRef hotnessCounter =
        Load(VariableType::INT32(), curMethod, IntPtr(Method::LITERAL_INFO_OFFSET));
    ProfileOperation callback;

    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(prevState, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label isBaselineBuiltinFrame(env);
    Label notBaselineBuiltinFrame(env);
    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label pcEqualBaseline(env);
    Label pcNotEqualBaseline(env);
    Label updateHotness(env);
    Label isStable(env);
    Label tryContinue(env);
    Label dispatch(env);
    Label slowPath(env);

    BRANCH(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &isStable);
    Bind(&isStable);
    {
        GateRef varProfileTypeInfoVal = *varProfileTypeInfo;
        GateRef isProfileDumpedAndJitCompiled = LogicAndBuilder(env)
            .And(ProfilerStubBuilder(env).IsProfileTypeInfoDumped(varProfileTypeInfoVal, callback))
            .And(ProfilerStubBuilder(env).IsCompiledOrTryCompile(
                glue, GetFunctionFromFrame(frame), varProfileTypeInfoVal, callback))
            .Done();
        BRANCH(isProfileDumpedAndJitCompiled, &tryContinue, &updateHotness);
    }
    Bind(&updateHotness);
    {
        GateRef offset = Int32Not(curPcOffset);
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, curMethod, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);

    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame,
                 IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = Load(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = Load(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(*prevState);
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { acc, *varSp, currentSp });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    BRANCH(IntPtrEqual(*varPc, IntPtr(BASELINEJIT_PC_FLAG)), &pcEqualBaseline, &pcNotEqualBaseline);
    Bind(&pcEqualBaseline);
    {
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { acc, *varSp, currentSp, jumpSize });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(*prevState);
        GateRef method = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                       { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
                         acc, *varHotnessCounter, jumpSize });
        Return();
    }
}

void BaselineLdlexvarImm8Imm8StubBuilder::GenerateCircuit()
{
    GateRef level = Int32Argument(PARAM_INDEX(BaselineLdlexvarImm8Imm8, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineLdlexvarImm8Imm8, SLOT));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdlexvarImm8Imm8, SP));

    auto env = GetEnvironment();
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
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, acc);
    Return();
}

// GLUE
void BaselineJnstricteqV8Imm16StubBuilder::GenerateCircuit()
{
    Return();
}

void BaselineAsyncgeneratorrejectV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineAsyncgeneratorrejectV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineAsyncgeneratorrejectV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineAsyncgeneratorrejectV8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineAsyncgeneratorrejectV8, V0));
    GateRef asyncGenerator = GetVregValue(sp, ZExtInt8ToPtr(v0));

    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncGeneratorReject),
                                 { asyncGenerator, acc });
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineSetgeneratorstateImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSetgeneratorstateImm8, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineSetgeneratorstateImm8, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineSetgeneratorstateImm8, INDEX));

    CallRuntime(glue, RTSTUB_ID(SetGeneratorState), { acc, IntToTaggedInt(index) });
    Return();
}

void BaselineGetasynciteratorImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineGetasynciteratorImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineGetasynciteratorImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineGetasynciteratorImm8, ACC));

    GateRef res = CallRuntime(glue, RTSTUB_ID(GetAsyncIterator), { acc });
    CHECK_PENDING_EXCEPTION(res);
}

void BaselineLdPrivatePropertyImm8Imm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, ACC));
    GateRef levelIndex = Int32Argument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, INDEX0));
    GateRef slotIndex = Int32Argument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, INDEX1));
    GateRef lexicalEnv = TaggedPointerArgument(PARAM_INDEX(BaselineLdPrivatePropertyImm8Imm16Imm16, ENV));

    GateRef res = CallRuntime(glue, RTSTUB_ID(LdPrivateProperty), {lexicalEnv,
        IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), acc});  // acc as obj
    CHECK_EXCEPTION_WITH_ACC(res);
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
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineTestInImm8Imm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, ACC));
    GateRef levelIndex = Int32Argument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, INDEX0));
    GateRef slotIndex = Int32Argument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, INDEX1));
    GateRef lexicalEnv = TaggedPointerArgument(PARAM_INDEX(BaselineTestInImm8Imm16Imm16, ENV));

    GateRef res = CallRuntime(glue, RTSTUB_ID(TestIn), {lexicalEnv,
        IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), acc});  // acc as obj
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineDeprecatedLdlexenvPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdlexenvPrefNone, SP));
    GateRef state = GetFrame(sp);

    GateRef env = GetEnvFromFrame(state);
    Return(env);
}

void BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, SP));
    GateRef numKeys = Int32Argument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, V0));
    GateRef objId = Int32Argument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, V1));
    GateRef firstArgRegIdx = Int32Argument(PARAM_INDEX(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8, V2));

    GateRef acc = GetAccFromFrame(GetFrame(sp));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(objId));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithExcludedKeys),
        { Int16ToTaggedInt(numKeys), obj, Int16ToTaggedInt(firstArgRegIdx) });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineThrowPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowPrefNone, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowPrefNone, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineThrowPrefNone, ACC));

    CallRuntime(glue, RTSTUB_ID(Throw), { acc });
    DISPATCH_LAST();
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
    GateRef numArgs = Int32Argument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, NUM_ARGS));
    GateRef firstArgRegIdx = Int32Argument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, IDX));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, HOTNESS_COUNTER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineWideNewobjrangePrefImm16V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);
    GateRef firstArgOffset = Int16(1);
    GateRef ctor = GetVregValue(sp, ZExtInt16ToPtr(firstArgRegIdx));

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
    Label noNeedCheckException(env);
    Label exit(env);

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
        {
            GateRef argv = PtrAdd(sp,
                PtrMul(PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
            GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8));
            JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV);
            callArgs.callConstructorArgs = { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj };
            CallStubBuilder callBuilder(this, glue, ctor, actualNumArgs, jumpSize, &res, hotnessCounter, callArgs,
                callback);
            callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
            Bind(&exit);
            Jump(&threadCheck);
        }
    }
    Bind(&slowPath);
    {
        GateRef firstArgIdx = Int16Add(firstArgRegIdx, firstArgOffset);
        GateRef length = Int16Sub(numArgs, firstArgOffset);
        res = CallRuntime(glue, RTSTUB_ID(NewObjRange),
                          { ctor, ctor, Int16ToTaggedInt(firstArgIdx), Int16ToTaggedInt(length) });
        Jump(&checkResult);
    }
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
        DISPATCH_LAST();
        Return(acc);
    }
    Bind(&dispatch);
    Return(*res);
    Bind(&noNeedCheckException);
    Return(*res);
}

void BaselineThrowNotexistsPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowNotexistsPrefNone, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowNotexistsPrefNone, SP));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    CallRuntime(glue, RTSTUB_ID(ThrowThrowNotExists), {});
    DISPATCH_LAST();
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

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetIteratorNext), { obj, method });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineWideNewlexenvPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideNewlexenvPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideNewlexenvPrefImm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideNewlexenvPrefImm16, ACC));
    GateRef numVars = Int32Argument(PARAM_INDEX(BaselineWideNewlexenvPrefImm16, NUM_VARS));
    GateRef state = GetFrame(sp);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    auto env = GetEnvironment();
    auto parent = GetEnvFromFrame(state);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterNew(env);
    newBuilder.NewLexicalEnv(&result, &afterNew, numVars, parent);
    Bind(&afterNew);
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(*result, &notException);
    Bind(&notException);
    SetEnvToFrame(glue, state, *result);
    Return(*result);
}

void BaselineThrowPatternnoncoerciblePrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowPatternnoncoerciblePrefNone, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowPatternnoncoerciblePrefNone, SP));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    CallRuntime(glue, RTSTUB_ID(ThrowPatternNonCoercible), {});
    DISPATCH_LAST();
    Return();
}

void BaselineDeprecatedCreatearraywithbufferPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, SP));
    GateRef immI16 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, IMM_I16));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, SLOT_ID));
    GateRef profileTypeInfo =
        TaggedPointerArgument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, PROFILE_TYPE_INFO));
    GateRef pc = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreatearraywithbufferPrefImm16, PC));
    GateRef imm = ZExtInt16ToInt32(immI16);

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef currentFunc = GetFunctionFromFrame(frame);
    ProfileOperation callback;
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, currentFunc, { pc, 0, true }, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineWideNewlexenvwithnamePrefImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, ACC));
    GateRef numVars = Int32Argument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, NUM_VARS));
    GateRef scopeId = Int32Argument(PARAM_INDEX(BaselineWideNewlexenvwithnamePrefImm16Id16, SCOPE_ID));

    auto env = GetEnvironment();
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewLexicalEnvWithName),
                              { Int16ToTaggedInt(numVars), Int16ToTaggedInt(scopeId) });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(res, &notException);
    Bind(&notException);
    GateRef state = GetFrame(sp);
    SetEnvToFrame(glue, state, res);
    Return(res);
}

void BaselineThrowDeletesuperpropertyPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowDeletesuperpropertyPrefNone, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowDeletesuperpropertyPrefNone, SP));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    CallRuntime(glue, RTSTUB_ID(ThrowDeleteSuperProperty), {});
    DISPATCH_LAST();
    Return();
}

void BaselineDeprecatedCreateobjectwithbufferPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreateobjectwithbufferPrefImm16, GLUE));
    GateRef immI16 = Int32Argument(PARAM_INDEX(BaselineDeprecatedCreateobjectwithbufferPrefImm16, IMM_I16));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCreateobjectwithbufferPrefImm16, SP));
    GateRef imm = ZExtInt16ToInt32(immI16);

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef module = GetModuleFromFunction(func);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithBuffer), { result });
    CHECK_EXCEPTION_WITH_ACC(res);
}

// GLUE, SP, NUM_ARGS, FIRST_ARG_REG_IDX
void BaselineNewobjrangeImm8Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, SP));
    GateRef numArgs = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, NUM_ARGS));
    GateRef firstArgRegIdx = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, FIRST_ARG_REG_IDX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm8Imm8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef firstArgOffset = Int32(1);
    GateRef ctor = GetVregValue(sp, ZExtInt32ToPtr(firstArgRegIdx));
    GateRef actualNumArgs = Int32Sub(numArgs, firstArgOffset);

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
    Label noNeedCheckException(env);
    Label exit(env);

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
        {
            GateRef argv = PtrAdd(sp, PtrMul(
                PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
            GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::IMM8_IMM8_V8));
            METHOD_ENTRY_ENV_DEFINED(ctor);
            JSCallArgs callArgs(JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV);
            callArgs.callConstructorArgs = { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj };
            CallStubBuilder callBuilder(this, glue, ctor, actualNumArgs, jumpSize, &res, hotnessCounter, callArgs,
                callback);
            callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
            Bind(&exit);
            Jump(&threadCheck);
        }
    }
    Bind(&slowPath);
    {
        GateRef firstArgIdx = Int32Add(firstArgRegIdx, firstArgOffset);
        GateRef length = Int32Sub(numArgs, firstArgOffset);
        res = CallRuntime(glue, RTSTUB_ID(NewObjRange),
                          { ctor, ctor, IntToTaggedInt(firstArgIdx), IntToTaggedInt(length) });
        Jump(&checkResult);
    }
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
        DISPATCH_LAST();
        Return(acc);
    }
    Bind(&dispatch);
    Return(*res);
    Bind(&noNeedCheckException);
    Return(*res);
}

// GLUE, SP, NUM_ARGS, FIRST_ARG_REG_IDX
void BaselineNewobjrangeImm16Imm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, SP));
    GateRef numArgs = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, NUM_ARGS));
    GateRef firstArgRegIdx = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, FIRST_ARG_REG_IDX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineNewobjrangeImm16Imm8V8, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
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
    Label noNeedCheckException(env);
    Label exit(env);

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
        {
            GateRef argv = PtrAdd(sp, PtrMul(
                PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
            GateRef jumpSize = IntPtr(-BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16_IMM8_V8));
            METHOD_ENTRY_ENV_DEFINED(ctor);
            JSCallArgs callArgs(JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV);
            callArgs.callConstructorArgs = { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj };
            CallStubBuilder callBuilder(this, glue, ctor, actualNumArgs, jumpSize, &res, hotnessCounter, callArgs,
                callback);
            callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
            Bind(&exit);
            Jump(&threadCheck);
        }
    }
    Bind(&slowPath);
    {
        GateRef firstArgIdx = Int16Add(firstArgRegIdx, firstArgOffset);
        GateRef length = Int16Sub(numArgs, firstArgOffset);
        res = CallRuntime(glue, RTSTUB_ID(NewObjRange),
                          { ctor, ctor, Int16ToTaggedInt(firstArgIdx), Int16ToTaggedInt(length) });
        Jump(&checkResult);
    }
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
        DISPATCH_LAST();
        Return(acc);
    }
    Bind(&dispatch);
    Return(*res);
    Bind(&noNeedCheckException);
    Return(*res);
}

// GLUE, SP, ACC, ACTUAL_NUM_ARGS, VREG_ID, HOTNESS_COUNTER
void BaselineWideCallrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, ACC));
    GateRef actualNumArgs = Int32Argument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, ACTUAL_NUM_ARGS));
    GateRef vregId = Int32Argument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, VREG_ID));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineWideCallrangePrefImm16V8, HOTNESS_COUNTER));
    ProfileOperation callback;
    GateRef argv = PtrAdd(sp, PtrMul(ZExtInt8ToPtr(vregId), IntPtr(8))); // 8: byteSize

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef jumpSize = INT_PTR(WIDE_CALLRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::CALL_WITH_ARGV);
    callArgs.callArgv = { numArgs, argv };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineThrowConstassignmentPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowConstassignmentPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowConstassignmentPrefV8, SP));
    GateRef v0 = TaggedArgument(PARAM_INDEX(BaselineThrowConstassignmentPrefV8, V0));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    CallRuntime(glue, RTSTUB_ID(ThrowConstAssignment), { v0 });
    DISPATCH_LAST();
    Return();
}

void BaselineDeprecatedTonumberPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedTonumberPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedTonumberPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedTonumberPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

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
        GateRef result = CallRuntime(glue, RTSTUB_ID(ToNumber), { value });
        CHECK_EXCEPTION_RETURN(result);
    }
}

void BaselineWideCallthisrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, ACC));
    GateRef actualNumArgs = Int32Argument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, ACTUAL_NUM_ARGS));
    GateRef vregId = Int32Argument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, VREG_ID));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineWideCallthisrangePrefImm16V8, HOTNESS_COUNTER));

    GateRef thisReg = ZExtInt8ToPtr(vregId);
    GateRef thisValue = GetVregValue(sp, thisReg);
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(thisReg, IntPtr(1)), IntPtr(8))); // 1: skip this
    ProfileOperation callback;

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef jumpSize = INT_PTR(WIDE_CALLTHISRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_WITH_ARGV);
    callArgs.callArgvWithThis = { numArgs, argv, thisValue };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineThrowIfnotobjectPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowIfnotobjectPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowIfnotobjectPrefV8, SP));
    GateRef v0 = TaggedArgument(PARAM_INDEX(BaselineThrowIfnotobjectPrefV8, V0));

    auto env = GetEnvironment();
    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    Label isEcmaObject(env);
    Label notEcmaObject(env);
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(v0), &isHeapObject, &notEcmaObject);
    Bind(&isHeapObject);
    Branch(TaggedObjectIsEcmaObject(v0), &isEcmaObject, &notEcmaObject);
    Bind(&isEcmaObject);
    {
        Return();
    }
    Bind(&notEcmaObject);
    CallRuntime(glue, RTSTUB_ID(ThrowIfNotObject), {});
    DISPATCH_LAST();
    Return();
}

void BaselineDeprecatedTonumericPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedTonumericPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedTonumericPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedTonumericPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    Label valueIsNumeric(env);
    Label valueNotNumeric(env);
    Branch(TaggedIsNumeric(value), &valueIsNumeric, &valueNotNumeric);
    Bind(&valueIsNumeric);
    {
        Return(value);
    }
    Bind(&valueNotNumeric);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(ToNumeric), { value });
        CHECK_EXCEPTION_RETURN(result);
    }
}

void BaselineWideSupercallthisrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideSupercallthisrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideSupercallthisrangePrefImm16V8, SP));
    GateRef range = Int32Argument(PARAM_INDEX(BaselineWideSupercallthisrangePrefImm16V8, RANGE));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineWideSupercallthisrangePrefImm16V8, V0));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef currentFunc = GetFunctionFromFrame(frame);
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { currentFunc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineThrowUndefinedifholePrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowUndefinedifholePrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowUndefinedifholePrefV8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineThrowUndefinedifholePrefV8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineThrowUndefinedifholePrefV8V8, V1));

    GateRef hole = GetVregValue(sp, ZExtInt8ToPtr(v0));
    auto env = GetEnvironment();
    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
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
    DISPATCH_LAST();
    Return();
}

void BaselineThrowUndefinedifholewithnamePrefId16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowUndefinedifholewithnamePrefId16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowUndefinedifholewithnamePrefId16, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineThrowUndefinedifholewithnamePrefId16, STRING_ID));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    auto env = GetEnvironment();
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(acc), &isHole, &notHole);
    Bind(&notHole);
    {
        Return();
    }
    Bind(&isHole);
    GateRef str = GetStringFromConstPool(glue, constpool, stringId);
    CallRuntime(glue, RTSTUB_ID(ThrowUndefinedIfHole), { str });
    DISPATCH_LAST();
    Return();
}

void BaselineDeprecatedNegPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedNegPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedNegPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedNegPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Neg(glue, value);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineWideSupercallarrowrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, ACC));
    GateRef range = Int32Argument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, RANGE));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineWideSupercallarrowrangePrefImm16V8, V0));

    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { acc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineThrowIfsupernotcorrectcallPrefImm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm8, ACC));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm8, IMM));

    GateRef res = CallRuntime(glue, RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        { Int8ToTaggedInt(imm), acc }); // acc is thisValue
    CHECK_EXCEPTION(res);
}

void BaselineDeprecatedNotPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedNotPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedNotPrefV8, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineDeprecatedNotPrefV8, INDEX));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(index));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Not(glue, value);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineWideLdobjbyindexPrefImm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideLdobjbyindexPrefImm32, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideLdobjbyindexPrefImm32, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideLdobjbyindexPrefImm32, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineWideLdobjbyindexPrefImm32, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    auto env = GetEnvironment();
    GateRef acc = GetAccFromFrame(frame);
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(acc), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, acc, index, callback);
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION_WITH_ACC(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { acc, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        CHECK_EXCEPTION_WITH_ACC(result);
    }
}

void BaselineThrowIfsupernotcorrectcallPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm16, ACC));
    GateRef imm = Int32Argument(PARAM_INDEX(BaselineThrowIfsupernotcorrectcallPrefImm16, IMM));

    GateRef res = CallRuntime(glue, RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        { Int16ToTaggedInt(imm), acc }); // acc is thisValue
    CHECK_EXCEPTION(res);
}

void BaselineDeprecatedIncPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedIncPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedIncPrefV8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedIncPrefV8, V0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Inc(glue, value);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineWideStobjbyindexPrefV8Imm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, V0));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineWideStobjbyindexPrefV8Imm32, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);

    GateRef receiver = GetVregValue(sp, ZExtInt32ToPtr(v0));
    GateRef acc = GetAccFromFrame(frame);
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
        CHECK_EXCEPTION(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        CHECK_EXCEPTION(result);
    }
}

void BaselineDeprecatedDecPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedDecPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedDecPrefV8, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineDeprecatedDecPrefV8, INDEX));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(index));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    OperationsStubBuilder builder(this);
    GateRef result = builder.Dec(glue, value);
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineWideStownbyindexPrefV8Imm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, V0));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, INDEX));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineWideStownbyindexPrefV8Imm32, SLOT_ID));
    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);
    GateRef receiver = GetVregValue(sp, ZExtInt32ToPtr(v0));

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
        CHECK_EXCEPTION(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        CHECK_EXCEPTION(result);
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
    GateRef acc = GetAccFromFrame(GetFrame(sp));

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG0);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARG0_PREF_V8);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_ARG0);
    callArgs.callArgs = { 0, 0, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineWideCopyrestargsPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideCopyrestargsPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideCopyrestargsPrefImm16, SP));
    GateRef restIdx = Int32Argument(PARAM_INDEX(BaselineWideCopyrestargsPrefImm16, INDEX));

    GateRef acc = GetAccFromFrame(GetFrame(sp));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CopyRestArgs), { IntToTaggedInt(restIdx) });
    CHECK_EXCEPTION_WITH_ACC(res);
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
    GateRef acc = GetAccFromFrame(GetFrame(sp));

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG1);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARG1_PREF_V8_V8);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_ARG1);
    callArgs.callArgs = { a0Value, 0, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineWideLdlexvarPrefImm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideLdlexvarPrefImm16Imm16, SP));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineWideLdlexvarPrefImm16Imm16, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineWideLdlexvarPrefImm16Imm16, SLOT));

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
    GateRef acc = GetAccFromFrame(GetFrame(sp));

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS2);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARGS2_PREF_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_ARG2);
    callArgs.callArgs = { a0Value, a1Value, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineWideStlexvarPrefImm16Imm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, ACC));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineWideStlexvarPrefImm16Imm16, SLOT));

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
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, acc);
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
    GateRef acc = GetAccFromFrame(GetFrame(sp));

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARGS3);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_ARG3);
    callArgs.callArgs = { a0Value, a1Value, a2Value };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineWideGetmodulenamespacePrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideGetmodulenamespacePrefImm16, GLUE));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideGetmodulenamespacePrefImm16, INDEX));

    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespaceByIndex), { Int16ToTaggedInt(index) });
    Return(moduleRef);
}

void BaselineDeprecatedCallrangePrefImm16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, SP));
    GateRef actualNumArgs = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, INDEX));
    GateRef funcReg = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, FUNC_REG));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineDeprecatedCallrangePrefImm16V8, HOTNESS_COUNTER));
    ProfileOperation callback;
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(ZExtInt8ToPtr(funcReg), IntPtr(1)), IntPtr(8))); // 1: skip function
    GateRef acc = GetAccFromFrame(GetFrame(sp));

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_WITH_ARGV);
    callArgs.callArgv = { numArgs, argv };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineWideStmodulevarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideStmodulevarPrefImm16, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideStmodulevarPrefImm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideStmodulevarPrefImm16, INDEX));

    CallRuntime(glue, RTSTUB_ID(StModuleVarByIndex), { Int16ToTaggedInt(index), acc });
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

    GateRef acc = GetAccFromFrame(GetFrame(sp));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CallSpread), { func, obj, array });
    CHECK_PENDING_EXCEPTION(res);
}

void BaselineWideLdlocalmodulevarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideLdlocalmodulevarPrefImm16, GLUE));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideLdlocalmodulevarPrefImm16, INDEX));

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
    GateRef acc = GetAccFromFrame(GetFrame(sp));

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef actualNumArgs = Int32Sub(index, Int32(1));  // 1: exclude this
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV);
    callArgs.callArgvWithThis = { numArgs, argv, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
}

void BaselineWideLdexternalmodulevarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideLdexternalmodulevarPrefImm16, GLUE));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideLdexternalmodulevarPrefImm16, INDEX));

    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdExternalModuleVarByIndex), { Int16ToTaggedInt(index) });
    Return(moduleRef);
}

// GLUE, SP, METHOD_ID, LITERAL_ID, LENGTH, VREG_IDS
void BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, SP));
    GateRef methodId = Int32Argument(
        PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, METHOD_ID));
    GateRef literalId = Int32Argument(
        PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, LITERAL_ID));
    GateRef length = Int32Argument(
        PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, LENGTH));
    GateRef vregIds = Int32Argument(
        PARAM_INDEX(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8, VREG_IDS));

    GateRef vRegId0 = Int32And(vregIds, Int32(ONE_BYTE_ALL_ONE));
    GateRef vregId1 = Int32And(Int32LSR(vregIds, Int32(ONE_BYTE_SIZE)), Int32(ONE_BYTE_ALL_ONE));

    GateRef frame = GetFrame(sp);
    GateRef currentFunc = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(currentFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef lexicalEnv = GetVregValue(sp, ZExtInt32ToPtr(vRegId0));
    GateRef proto = GetVregValue(sp, ZExtInt32ToPtr(vregId1));

    GateRef acc = GetAccFromFrame(frame);
    auto env = GetEnvironment();
    GateRef module = GetModuleFromFunction(currentFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer),
                              { proto, lexicalEnv, constpool,
                                IntToTaggedInt(methodId),
                                IntToTaggedInt(literalId), module,
                                IntToTaggedInt(length)});

    Label isException(env);
    Label isNotException(env);
    Branch(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
        Return(res);
    }
    Bind(&isNotException);
    Return(res);
}

void BaselineWideLdpatchvarPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineWideLdpatchvarPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideLdpatchvarPrefImm16, SP));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideLdpatchvarPrefImm16, INDEX));

    GateRef result = CallRuntime(glue, RTSTUB_ID(LdPatchVar), { Int16ToTaggedInt(index) });
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineDeprecatedResumegeneratorPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedResumegeneratorPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedResumegeneratorPrefV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedResumegeneratorPrefV8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedResumegeneratorPrefV8, V0));
    (void)glue;
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));

    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef curFunc = GetFunctionFromFrame(GetFrame(sp));
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
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineWideStpatchvarPrefImm16, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineWideStpatchvarPrefImm16, ACC));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineWideStpatchvarPrefImm16, INDEX));

    GateRef result = CallRuntime(glue, RTSTUB_ID(StPatchVar), { Int16ToTaggedInt(index), acc });
    CHECK_EXCEPTION(result);
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

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineDeprecatedDelobjpropPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedDelobjpropPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedDelobjpropPrefV8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedDelobjpropPrefV8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedDelobjpropPrefV8V8, V1));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef result = CallRuntime(glue, RTSTUB_ID(DelObjProp), { obj, prop });
    CHECK_EXCEPTION_WITH_ACC(result);
}

// GLUE, SP, PC, V0, V1
void BaselineDeprecatedSuspendgeneratorPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedSuspendgeneratorPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedSuspendgeneratorPrefV8V8, SP));
    GateRef curPcOffset = Int32Argument(PARAM_INDEX(BaselineDeprecatedSuspendgeneratorPrefV8V8, OFFSET));
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
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(prevState, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
    GateRef genObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));

    Label isBaselineBuiltinFrame(env);
    Label notBaselineBuiltinFrame(env);
    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label pcEqualBaseline(env);
    Label pcNotEqualBaseline(env);
    Label updateHotness(env);
    Label isStable(env);
    Label tryContinue(env);
    Label dispatch(env);
    Label slowPath(env);

    GateRef res = CallRuntime(glue, RTSTUB_ID(SuspendGenerator), { genObj, value });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
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
        GateRef offset = Int32Not(curPcOffset);
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, method, *varHotnessCounter);
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
    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = Load(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = Load(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    GateRef varPc = GetPcFromFrame(*prevState);
    Branch(IntPtrEqual(varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { currentSp });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    BRANCH(IntPtrEqual(varPc, IntPtr(BASELINEJIT_PC_FLAG)), &pcEqualBaseline, &pcNotEqualBaseline);
    Bind(&pcEqualBaseline);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { *varAcc, *varSp, currentSp });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(*prevState);
        GateRef prevMethod = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(prevMethod);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(prevMethod);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
            { glue, currentSp, varPc, *varConstpool, *varProfileTypeInfo,
            *varAcc, *varHotnessCounter, jumpSize });
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

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncFunctionAwaitUncaught), { asyncFuncObj, value });
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineDeprecatedCopydatapropertiesPrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedCopydatapropertiesPrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedCopydatapropertiesPrefV8V8, SP));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedCopydatapropertiesPrefV8V8, V0));
    GateRef v1 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedCopydatapropertiesPrefV8V8, V1));
    GateRef dst = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef src = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CopyDataProperties), { dst, src });
    CHECK_EXCEPTION_WITH_ACC(result);
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
    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, obj });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP_RETURN(result, &notException);
    Bind(&notException);
    Return(result);
}

void BaselineDeprecatedLdobjbyvaluePrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, SP));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, V0));
    GateRef v1 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbyvaluePrefV8V8, V1));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.DeprecatedLoadObjByValue(glue, receiver, propKey);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineDeprecatedLdsuperbyvaluePrefV8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbyvaluePrefV8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbyvaluePrefV8V8, SP));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdsuperbyvaluePrefV8V8, V0));
    GateRef v1 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdsuperbyvaluePrefV8V8, V1));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));

    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, propKey }); // sp for thisFunc
    CHECK_EXCEPTION_WITH_ACC(result);
}

void BaselineDeprecatedLdobjbyindexPrefV8Imm32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyindexPrefV8Imm32, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbyindexPrefV8Imm32, SP));
    GateRef v0 =  Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbyindexPrefV8Imm32, V0));
    GateRef index = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbyindexPrefV8Imm32, INDEX));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
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
        CHECK_EXCEPTION_WITH_ACC(result);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        CHECK_EXCEPTION_WITH_ACC(result);
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

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedTrue() });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8, V0));
    GateRef v1 = Int32Argument(PARAM_INDEX(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8, V1));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedFalse() });
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineDeprecatedStlexvarPrefImm4Imm4V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, SP));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, SLOT));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm4Imm4V8, V0));

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
    GateRef level = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm8Imm8V8, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm8Imm8V8, SLOT));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm8Imm8V8, V0));

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
    GateRef level = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm16Imm16V8, LEVEL));
    GateRef slot = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm16Imm16V8, SLOT));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlexvarPrefImm16Imm16V8, V0));

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

void BaselineDeprecatedGetmodulenamespacePrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedGetmodulenamespacePrefId32, GLUE));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedGetmodulenamespacePrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedGetmodulenamespacePrefId32, SP));

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef prop = GetStringFromConstPool(glue, constpool, stringId);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespace), { prop });
    Return(moduleRef);
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

    CallRuntime(glue, RTSTUB_ID(StModuleVar), { prop, acc });
    Return();
}

void BaselineDeprecatedLdobjbynamePrefId32V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, SP));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, V0));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdobjbynamePrefId32V8, STRING_ID));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.DeprecatedLoadObjByName(glue, receiver, propKey);
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineDeprecatedLdsuperbynamePrefId32V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, SP));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, STRING_ID));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdsuperbynamePrefId32V8, V0));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { receiver, propKey });
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineDeprecatedLdmodulevarPrefId32Imm8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, GLUE));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, STRING_ID));
    GateRef flagI8 = Int32Argument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, FLAG_I8));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdmodulevarPrefId32Imm8, SP));
    GateRef flag = ZExtInt8ToInt32(flagI8);

    GateRef func = GetFunctionFromFrame(GetFrame(sp));
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef key = GetStringFromConstPool(glue, constpool, stringId);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdModuleVar), { key, IntToTaggedInt(flag) });
    Return(moduleRef);
}

void BaselineDeprecatedStconsttoglobalrecordPrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStconsttoglobalrecordPrefId32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedStconsttoglobalrecordPrefId32, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedStconsttoglobalrecordPrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStconsttoglobalrecordPrefId32, SP));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, acc, TaggedTrue() });
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineDeprecatedStlettoglobalrecordPrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlettoglobalrecordPrefId32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedStlettoglobalrecordPrefId32, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedStlettoglobalrecordPrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStlettoglobalrecordPrefId32, SP));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, acc, TaggedFalse() });
    CHECK_EXCEPTION_RETURN(result);
}

void BaselineDeprecatedStclasstoglobalrecordPrefId32StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedStclasstoglobalrecordPrefId32, GLUE));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedStclasstoglobalrecordPrefId32, ACC));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDeprecatedStclasstoglobalrecordPrefId32, STRING_ID));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedStclasstoglobalrecordPrefId32, SP));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, acc, TaggedFalse() });
    CHECK_EXCEPTION_RETURN(result);
}

// SP
void BaselineDeprecatedLdhomeobjectPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedLdhomeobjectPrefNone, SP));

    GateRef state = GetFunctionFromFrame(GetFrame(sp));
    GateRef result = GetHomeObjectFromJSFunction(state);
    Return(result);
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
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineDeprecatedDynamicimportPrefV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDeprecatedDynamicimportPrefV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDeprecatedDynamicimportPrefV8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineDeprecatedDynamicimportPrefV8, ACC));
    GateRef specifier = TaggedArgument(PARAM_INDEX(BaselineDeprecatedDynamicimportPrefV8, VREG));

    GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
    GateRef res = CallRuntime(glue, RTSTUB_ID(DynamicImport), { specifier, currentFunc });
    CHECK_EXCEPTION_WITH_ACC(res);
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

// GLUE, SP, SLOT_ID, STRING_ID, V0
void BaselineDefineFieldByNameImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, SLOT_ID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, STRING_ID));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDefineFieldByNameImm8Id16V8, V0));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetVregValue(sp, ZExtInt32ToPtr(v0));

    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFINE_BY_NAME(Boolean(false));
    CHECK_EXCEPTION_WITH_ACC(*result);
}


// GLUE, SP, SLOT_ID, STRING_ID, V0
void BaselineDefinePropertyByNameImm8Id16V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, SP));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, SLOT_ID));
    GateRef stringId = Int32Argument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, STRING_ID));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineDefinePropertyByNameImm8Id16V8, V0));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    GateRef acc = GetAccFromFrame(frame);
    GateRef method = GetMethodFromFunction(curFunc);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef receiver = GetVregValue(sp, ZExtInt32ToPtr(v0));

    auto env = GetEnvironment();
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFINE_BY_NAME(Boolean(true));
    CHECK_EXCEPTION_WITH_ACC(*result);
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

    GateRef res = DefineField(glue, obj, propKey, acc);
    CHECK_EXCEPTION_WITH_ACC(res);
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
    GateRef res = DefineField(glue, obj, propKey, acc);
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCallRuntimeToPropertyKeyPrefNoneStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeToPropertyKeyPrefNone, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeToPropertyKeyPrefNone, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeToPropertyKeyPrefNone, ACC));

    GateRef res = CallRuntime(glue, RTSTUB_ID(ToPropertyKey), { acc });
    CHECK_EXCEPTION_WITH_ACC(res);
}

// GLUE, SP, COUNT, LITERAL_ID
void BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16, SP));
    GateRef count = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16, COUNT));
    GateRef literalId = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16, LITERAL_ID));

    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef lexicalEnv = GetEnvFromFrame(frame);
    GateRef module = GetModuleFromFunction(func);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreatePrivateProperty), {lexicalEnv,
        IntToTaggedInt(count), constpool, IntToTaggedInt(literalId), module});
    CHECK_EXCEPTION_WITH_ACC(res);
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
    CHECK_EXCEPTION_WITH_ACC(res);
}

void BaselineCallRuntimeCallInitPrefImm8V8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, SP));
    GateRef acc = TaggedArgument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, ACC));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, V0));
    GateRef hotnessCounter = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, HOTNESS_COUNTER));
    GateRef slotId = Int32Argument(PARAM_INDEX(BaselineCallRuntimeCallInitPrefImm8V8, SLOT_ID));
    GateRef thisValue = GetVregValue(sp, ZExtInt8ToPtr(v0));

    DEFINE_PROFILE_CALLBACK(glue, sp, slotId);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    // same as callthis0
    GateRef actualNumArgs = Int32(EcmaInterpreter::ActualNumArgsOfCall::CALLARG0);
    METHOD_ENTRY(acc);
    Label noNeedCheckException(env);
    Label exit(env);
    GateRef jumpSize = INT_PTR(CALLRUNTIME_CALLINIT_PREF_IMM8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG0);
    callArgs.callArgsWithThis = { 0, 0, 0, thisValue };
    CallStubBuilder callBuilder(this, glue, acc, actualNumArgs, jumpSize, &result, hotnessCounter, callArgs, callback);
    callBuilder.JSCallDispatchForBaseline(&exit, &noNeedCheckException);
    Bind(&exit);
    CHECK_PENDING_EXCEPTION(*result);
    Bind(&noNeedCheckException);
    Return(*result);
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

    auto env = GetEnvironment();
    GateRef module = GetModuleFromFunction(currentFunc);
    // remove constpool in args
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateSharedClass),
                              { proto,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId),
                                Int16ToTaggedInt(length), module });

    Label isException(env);
    Label isNotException(env);
    BRANCH(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        GateRef acc = GetAccFromFrame(frame);
        DISPATCH_LAST_WITH_ACC();
        Return(res);
    }
    Bind(&isNotException);
    Return(res);
}

void BaselineCallRuntimeLdSendableClassPrefImm16StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineCallRuntimeLdSendableClassPrefImm16, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineCallRuntimeLdSendableClassPrefImm16, SP));
    GateRef level = Int32Argument(PARAM_INDEX(BaselineCallRuntimeLdSendableClassPrefImm16, LEVEL));
    GateRef lexEnv = GetEnvFromFrame(GetFrame(sp));

    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSendableClass), { lexEnv, Int16ToTaggedInt(level) });
    Return(result);
}

void BaselineReturnundefinedStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineReturnundefined, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineReturnundefined, SP));
    GateRef curPcOffset = Int32Argument(PARAM_INDEX(BaselineReturnundefined, OFFSET));

    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    GateRef frame = GetFrame(sp);
    GateRef acc = GetAccFromFrame(frame);

    auto env = GetEnvironment();
    GateRef pc = GetPcFromFrame(frame);
    GateRef curFunction = GetFunctionFromFrame(frame);
    GateRef curMethod =
        Load(VariableType::JS_ANY(), curFunction, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef constpool =
        Load(VariableType::JS_POINTER(), curMethod, IntPtr(Method::CONSTANT_POOL_OFFSET));
    GateRef raw =
        Load(VariableType::JS_POINTER(), curFunction, IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    GateRef profileTypeInfo =
        Load(VariableType::JS_POINTER(), raw, IntPtr(ProfileTypeInfoCell::VALUE_OFFSET));
    GateRef hotnessCounter =
        Load(VariableType::INT16(), curMethod, IntPtr(Method::LITERAL_INFO_OFFSET));
    ProfileOperation callback;

    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(prevState, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label isBaselineBuiltinFrame(env);
    Label notBaselineBuiltinFrame(env);
    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label pcEqualBaseline(env);
    Label pcNotEqualBaseline(env);
    Label updateHotness(env);
    Label isStable(env);
    Label tryContinue(env);
    Label dispatch(env);
    Label slowPath(env);

    BRANCH(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &isStable);
    Bind(&isStable);
    {
        GateRef varProfileTypeInfoVal = *varProfileTypeInfo;
        GateRef isProfileDumpedAndJitCompiled = LogicAndBuilder(env)
            .And(ProfilerStubBuilder(env).IsProfileTypeInfoDumped(varProfileTypeInfoVal, callback))
            .And(ProfilerStubBuilder(env).IsCompiledOrTryCompile(
                glue, GetFunctionFromFrame(frame), varProfileTypeInfoVal, callback))
            .Done();
        BRANCH(isProfileDumpedAndJitCompiled, &tryContinue, &updateHotness);
    }
    Bind(&updateHotness);
    {
        GateRef offset = Int32Not(curPcOffset);
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, curMethod, *varHotnessCounter);
        Jump(&tryContinue);
    }
    Bind(&tryContinue);
    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame,
                 IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = Load(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = Load(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(*prevState);
    varAcc = Undefined();
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc, *varSp, currentSp });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    BRANCH(IntPtrEqual(*varPc, IntPtr(BASELINEJIT_PC_FLAG)), &pcEqualBaseline, &pcNotEqualBaseline);
    Bind(&pcEqualBaseline);
    {
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { *varAcc, *varSp, currentSp, jumpSize });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(*prevState);
        GateRef method = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                       { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
                         *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

void BaselineSuspendgeneratorV8StubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineSuspendgeneratorV8, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineSuspendgeneratorV8, SP));
    GateRef curPcOffset = PtrArgument(PARAM_INDEX(BaselineSuspendgeneratorV8, OFFSET));
    GateRef v0 = Int32Argument(PARAM_INDEX(BaselineSuspendgeneratorV8, V0));
    ProfileOperation callback;

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef method = GetMethodFromFunction(func);
    GateRef constpool = GetConstpoolFromMethod(method);
    GateRef acc = GetAccFromFrame(frame);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);

    auto env = GetEnvironment();
    METHOD_EXIT();
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(prevState, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label isBaselineBuiltinFrame(env);
    Label notBaselineBuiltinFrame(env);
    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label pcEqualBaseline(env);
    Label pcNotEqualBaseline(env);
    Label updateHotness(env);
    Label isStable(env);
    Label tryContinue(env);
    Label dispatch(env);
    Label slowPath(env);

    GateRef genObj = GetVregValue(sp, ZExtInt32ToPtr(v0));
    GateRef value = *varAcc;
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuspendGenerator), { genObj, value });
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
        Return();
    }
    Bind(&notException);
    varAcc = res;
    BRANCH(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &isStable);
    Bind(&isStable);
    {
        BRANCH(ProfilerStubBuilder(env).IsProfileTypeInfoDumped(*varProfileTypeInfo, callback), &tryContinue,
            &updateHotness);
    }
    Bind(&updateHotness);
    {
        GateRef offset = Int32Not(curPcOffset);
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, method, *varHotnessCounter);
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

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = Load(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = Load(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    GateRef varPc = GetPcFromFrame(*prevState);
    BRANCH(IntPtrEqual(varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc, *varSp, currentSp });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    BRANCH(IntPtrEqual(varPc, IntPtr(BASELINEJIT_PC_FLAG)), &pcEqualBaseline, &pcNotEqualBaseline);
    Bind(&pcEqualBaseline);
    {
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { *varAcc, *varSp, currentSp, jumpSize });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(*prevState);
        GateRef prevMethod = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(prevMethod);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varHotnessCounter = GetHotnessCounterFromMethod(prevMethod);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
            { glue, currentSp, varPc, *varConstpool, *varProfileTypeInfo,
            *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

// interpreter helper handler
void BaselineExceptionHandlerStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(PARAM_INDEX(BaselineExceptionHandler, GLUE));
    GateRef sp = PtrArgument(PARAM_INDEX(BaselineExceptionHandler, SP));
    GateRef acc = PtrArgument(PARAM_INDEX(BaselineExceptionHandler, ACC));

    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(frame);
    GateRef profileTypeInfo = GetProfileTypeInfoFromFunction(func);
    GateRef method = GetMethodFromFunction(func);
    GateRef hotnessCounter = GetHotnessCounterFromMethod(method);
    GateRef constpool = GetConstpoolFromMethod(method);

    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), IntPtr(0));
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
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
    GateRef acc = GetAccFromFrame(frame);

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
        BRANCH(BitOr(TaggedIsUndefined(*varProfileTypeInfo),
                     Int8Equal(interruptsFlag, Int8(VmThreadControl::VM_NEED_SUSPENSION))),
            &callRuntime, &initialized);
        Bind(&callRuntime);
        varProfileTypeInfo = CallRuntime(glue, RTSTUB_ID(UpdateHotnessCounterWithProf), { func });

        Label handleException(env);
        BRANCH(HasPendingException(glue), &handleException, &exitLabel);
        Bind(&handleException);
        {
            DISPATCH_LAST();
            Return();
        }
        Bind(&initialized);
        ProfilerStubBuilder profiler(this);
        profiler.TryJitCompile(glue, { offset, 0, false }, func, profileTypeInfo);
        Jump(&exitLabel);
    }
    Bind(&exitLabel);
    SetHotnessCounter(glue, method, *varHotnessCounter);
    Return();
}
#undef UPDATE_HOTNESS
#undef PARAM_INDEX
#undef DISPATCH_LAST
#undef DISPATCH_LAST_WITH_ACC
}  // namespace panda::ecmascript::kungfu
