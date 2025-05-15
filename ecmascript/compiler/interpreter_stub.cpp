/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/profiler_stub_builder.h"
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/interpreter/interpreter_assembly.h"

namespace panda::ecmascript::kungfu {
#define DECLARE_ASM_HANDLER_BASE(name, needPrint, V, format)                                      \
void name##StubBuilder::GenerateCircuit()                                                         \
{                                                                                                 \
    GateRef glue = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::GLUE));              \
    GateRef sp = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::SP));                  \
    GateRef pc = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::PC));                  \
    GateRef constpool = TaggedPointerArgument(                                                    \
        static_cast<size_t>(InterpreterHandlerInputs::CONSTPOOL));                                \
    GateRef profileTypeInfo = TaggedPointerArgument(                                              \
        static_cast<size_t>(InterpreterHandlerInputs::PROFILE_TYPE_INFO));                        \
    GateRef acc = TaggedArgument(static_cast<size_t>(InterpreterHandlerInputs::ACC));             \
    GateRef hotnessCounter = Int32Argument(                                                       \
        static_cast<size_t>(InterpreterHandlerInputs::HOTNESS_COUNTER));                          \
    DebugPrintInstruction<needPrint>();                                                           \
    V(format)                                                                                     \
    GenerateCircuitImpl(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, callback); \
}

#define DECLARE_ASM_HANDLE_IMPLEMENT(name)                                           \
void name##StubBuilder::GenerateCircuitImpl(GateRef glue, GateRef sp, GateRef pc,    \
                                     GateRef constpool, GateRef profileTypeInfo,     \
                                     GateRef acc, GateRef hotnessCounter,            \
                                     [[maybe_unused]] ProfileOperation callback)

#define REGISTER_PROFILE_CALL_BACK(format)                                                                             \
    ProfileOperation callback(                                                                                         \
        [this, glue, sp, pc, profileTypeInfo](const std::initializer_list<GateRef> &values, OperationType type) {      \
            ProfilerStubBuilder profiler(this);                                                                        \
            profiler.PGOProfiler(glue, pc, GetFunctionFromFrame(glue, GetFrame(sp)),                                   \
                profileTypeInfo, values, format, type);                                                                \
        }, nullptr);

#define REGISTER_JIT_PROFILE_CALL_BACK(format)                                                                         \
    ProfileOperation callback(                                                                                         \
        nullptr,                                                                                                       \
        [this, glue, sp, pc, profileTypeInfo](const std::initializer_list<GateRef> &values, OperationType type) {      \
            ProfilerStubBuilder profiler(this);                                                                        \
            profiler.PGOProfiler(glue, pc, GetFunctionFromFrame(glue, GetFrame(sp)),                                   \
                profileTypeInfo, values, format, type);                                                                \
        });

#define REGISTER_NULL_CALL_BACK(format) ProfileOperation callback;

#define DECLARE_ASM_HANDLER(name)                                                     \
    DECLARE_ASM_HANDLER_BASE(name, true, REGISTER_NULL_CALL_BACK, SlotIDFormat::IMM8) \
    DECLARE_ASM_HANDLE_IMPLEMENT(name)

#define DECLARE_ASM_HANDLER_NOPRINT(name)                                              \
    DECLARE_ASM_HANDLER_BASE(name, false, REGISTER_NULL_CALL_BACK, SlotIDFormat::IMM8) \
    DECLARE_ASM_HANDLE_IMPLEMENT(name)

#define DECLARE_ASM_HANDLER_PROFILE(name, base, format) \
    DECLARE_ASM_HANDLER_BASE(name, true, REGISTER_PROFILE_CALL_BACK, format)

#define DECLARE_ASM_HANDLER_JIT_PROFILE(name, base, format) \
    DECLARE_ASM_HANDLER_BASE(name, true, REGISTER_JIT_PROFILE_CALL_BACK, format)

// TYPE:{OFFSET, ACC_VARACC, JUMP, SSD}
#define DISPATCH_BAK(TYPE, ...) DISPATCH_##TYPE(__VA_ARGS__)

// Dispatch(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, offset)
#define DISPATCH_OFFSET(offset)                                                           \
    DISPATCH_BASE(profileTypeInfo, acc, hotnessCounter, offset)

// Dispatch(glue, sp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter, offset)
#define DISPATCH_VARACC(offset)                                                           \
    DISPATCH_BASE(profileTypeInfo, *varAcc, hotnessCounter, offset)

// Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, offset)
#define DISPATCH_JUMP(offset)                                                             \
    DISPATCH_BASE(*varProfileTypeInfo, acc, *varHotnessCounter, offset)

#define DISPATCH_SSD(offset)                                                              \
    Dispatch(glue, *varSp, *varPc, *varConstpool, *varProfileTypeInfo, *varAcc,           \
             *varHotnessCounter, offset)

#define DISPATCH_BASE(...)                                                                \
    Dispatch(glue, sp, pc, constpool, __VA_ARGS__)

#define INT_PTR(opcode)                                                                   \
    IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Opcode::opcode))

#define DISPATCH_WITH_ACC(opcode) DISPATCH_BAK(VARACC, INT_PTR(opcode))

#define DISPATCH(opcode) DISPATCH_BAK(OFFSET, INT_PTR(opcode))

#define DISPATCH_LAST()                                                                   \
    DispatchLast(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter)           \

#define DISPATCH_LAST_WITH_ACC()                                                          \
    DispatchLast(glue, sp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter)       \

#define UPDATE_HOTNESS(_sp, callback)                                                                          \
    varHotnessCounter = Int32Add(offset, *varHotnessCounter);                                                  \
    BRANCH(Int32LessThan(*varHotnessCounter, Int32(0)), &slowPath, &dispatch);                                 \
    Bind(&slowPath);                                                                                           \
    {                                                                                                          \
        GateRef func = GetFunctionFromFrame(glue, GetFrame(_sp));                                                    \
        GateRef iVecOffset = IntPtr(JSThread::GlueData::GetInterruptVectorOffset(env->IsArch32Bit()));         \
        GateRef interruptsFlag = LoadPrimitive(VariableType::INT8(), glue, iVecOffset);                        \
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

#define CHECK_EXCEPTION(res, offset)                                                      \
    CheckException(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter,         \
                   res, offset)

#define CHECK_EXCEPTION_VARACC(res, offset)                                               \
    CheckException(glue, sp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter,     \
                   res, offset)

#define CHECK_EXCEPTION_WITH_JUMP(res, jump)                                              \
    CheckExceptionWithJump(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, \
		           res, jump)

#define CHECK_EXCEPTION_WITH_ACC(res, offset)                                             \
    CheckExceptionWithVar(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter,  \
		          res, offset)

#define CHECK_EXCEPTION_WITH_VARACC(res, offset)                                              \
    CheckExceptionWithVar(glue, sp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter,  \
		          res, offset)

#define CHECK_PENDING_EXCEPTION(res, offset)                                              \
    CheckPendingException(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter,  \
		          res, offset)

#define METHOD_ENTRY(func)                                                                        \
    auto env = GetEnvironment();                                                                  \
    METHOD_ENTRY_ENV_DEFINED(func)

#define METHOD_ENTRY_ENV_DEFINED(func)                                                            \
    GateRef isDebugModeOffset = IntPtr(JSThread::GlueData::GetIsDebugModeOffset(env->Is32Bit())); \
    GateRef isDebugMode = LoadPrimitive(VariableType::BOOL(), glue, isDebugModeOffset);           \
    Label isDebugModeTrue(env);                                                                   \
    Label isDebugModeFalse(env);                                                                  \
    BRANCH(isDebugMode, &isDebugModeTrue, &isDebugModeFalse);                                     \
    Bind(&isDebugModeTrue);                                                                       \
    {                                                                                             \
        CallRuntime(glue, RTSTUB_ID(MethodEntry), { func });                                      \
        Jump(&isDebugModeFalse);                                                                  \
    }                                                                                             \
    Bind(&isDebugModeFalse)

#define METHOD_EXIT()                                                                                           \
    Label NeedCallRuntimeTrue(env);                                                                             \
    Label NeedCallRuntimeFalse(env);                                                                            \
    GateRef isDebugModeOrTracing = LogicOrBuilder(env)                                                          \
        .Or(LoadPrimitive(VariableType::BOOL(), glue,                                                           \
            IntPtr(JSThread::GlueData::GetIsDebugModeOffset(env->Is32Bit()))))                                  \
        .Or(LoadPrimitive(VariableType::BOOL(), glue,                                                           \
            IntPtr(JSThread::GlueData::GetIsTracingOffset(env->Is32Bit()))))                                    \
        .Done();                                                                                                \
    BRANCH(isDebugModeOrTracing, &NeedCallRuntimeTrue, &NeedCallRuntimeFalse);                                  \
    Bind(&NeedCallRuntimeTrue);                                                                                 \
    {                                                                                                           \
        CallRuntime(glue, RTSTUB_ID(MethodExit), {});                                                           \
        Jump(&NeedCallRuntimeFalse);                                                                            \
    }                                                                                                           \
    Bind(&NeedCallRuntimeFalse)

#define TRY_OSR()                                                                                                    \
    DEFVARIABLE(varOsrCache, VariableType::NATIVE_POINTER(), Undefined());                                           \
    DEFVARIABLE(varMachineCodeOffset, VariableType::JS_ANY(), Undefined());                                          \
    DEFVARIABLE(varMachineCode, VariableType::NATIVE_POINTER(), Undefined());                                        \
    Label getOsrCache(env);                                                                                          \
    Label getMachineCode(env);                                                                                       \
    Label checkDeOptFlag(env);                                                                                       \
    Label checkExecCount(env);                                                                                       \
    Label clearMachineCode(env);                                                                                     \
    Label executeBCByAOT(env);                                                                                       \
    Label executeBCByInterpreter(env);                                                                               \
    GateRef curFrame = GetFrame(sp);                                                                                 \
    GateRef curFunction = GetFunctionFromFrame(glue, curFrame);                                                      \
    GateRef curMethod = Load(VariableType::JS_ANY(), glue, curFunction, IntPtr(JSFunctionBase::METHOD_OFFSET));      \
    Branch(TaggedIsUndefined(profileTypeInfo), &executeBCByInterpreter, &getOsrCache);                               \
    Bind(&getOsrCache);                                                                                              \
    {                                                                                                                \
        GateRef profileTypeInfoLength = GetLengthOfTaggedArray(profileTypeInfo);                                     \
        GateRef typeInfoNum = Int32Sub(profileTypeInfoLength, Int32(ProfileTypeInfo::JIT_OSR_INDEX));                \
        GateRef relativeOffset = PtrMul(ZExtInt32ToPtr(typeInfoNum), IntPtr(JSTaggedValue::TaggedTypeSize()));       \
        GateRef osrCacheOffset = PtrAdd(relativeOffset, IntPtr(TaggedArray::DATA_OFFSET));                           \
        varOsrCache = LoadPrimitive(VariableType::NATIVE_POINTER(), profileTypeInfo, osrCacheOffset);                \
        Branch(TaggedIsUndefinedOrNull(*varOsrCache), &executeBCByInterpreter, &getMachineCode);                     \
    }                                                                                                                \
    Bind(&getMachineCode);                                                                                           \
    {                                                                                                                \
        DEFVARIABLE(varIndex, VariableType::INT32(), Int32(0));                                                      \
        Label traverseOsrCache(env);                                                                                 \
        Label compareOffset(env);                                                                                    \
        Label addIndex(env);                                                                                         \
        Label traverseOsrCacheAgain(env);                                                                            \
        GateRef fistPC = LoadPrimitive(VariableType::NATIVE_POINTER(), curMethod,                                    \
                              IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));                              \
        GateRef jmpOffsetInFunc = TruncPtrToInt32(PtrSub(pc, fistPC));                                               \
        GateRef length = GetLengthOfTaggedArray(*varOsrCache);                                                       \
        Branch(Int32LessThan(*varIndex, length), &traverseOsrCache, &executeBCByInterpreter);                        \
        LoopBegin(&traverseOsrCache);                                                                                \
        {                                                                                                            \
            GateRef relativeOffset = PtrMul(ZExtInt32ToPtr(*varIndex), IntPtr(JSTaggedValue::TaggedTypeSize()));     \
            varMachineCodeOffset = PtrAdd(relativeOffset, IntPtr(TaggedArray::DATA_OFFSET));                         \
            varMachineCode = LoadPrimitive(VariableType::NATIVE_POINTER(), *varOsrCache, *varMachineCodeOffset);     \
            Branch(TaggedIsUndefinedOrNull(*varMachineCode), &addIndex, &compareOffset);                             \
            Bind(&compareOffset);                                                                                    \
            {                                                                                                        \
                GateRef offsetField = LoadPrimitive(VariableType::INT32(), *varMachineCode,                          \
                                           IntPtr(MachineCode::INS_SIZE_OFFSET));                                    \
                Branch(Int32Equal(jmpOffsetInFunc, offsetField), &checkExecCount, &addIndex);                        \
            }                                                                                                        \
            Bind(&addIndex);                                                                                         \
            {                                                                                                        \
                varIndex = Int32Add(*varIndex, Int32(1));                                                            \
                varMachineCode = NullPtr();                                                                          \
                Branch(Int32LessThan(*varIndex, length), &traverseOsrCacheAgain, &executeBCByInterpreter);           \
            }                                                                                                        \
            Bind(&traverseOsrCacheAgain);                                                                            \
        }                                                                                                            \
        LoopEnd(&traverseOsrCache);                                                                                  \
    }                                                                                                                \
    Bind(&checkExecCount);                                                                                           \
    {                                                                                                                \
        GateRef execCnt =                                                                                            \
            LoadPrimitive(VariableType::INT16(), *varMachineCode, IntPtr(MachineCode::OSR_EXECUTE_CNT_OFFSET));      \
        Branch(Int32LessThan(ZExtInt16ToInt32(execCnt), Int32(5)), &checkDeOptFlag, &dispatch);                      \
    }                                                                                                                \
    Bind(&checkDeOptFlag);                                                                                           \
    {                                                                                                                \
        GateRef deOptField =                                                                                         \
            LoadPrimitive(VariableType::INT16(), *varMachineCode, IntPtr(MachineCode::OSRMASK_OFFSET));              \
        Branch(Equal(deOptField, Int16(MachineCode::OSR_DEOPT_FLAG)), &clearMachineCode, &executeBCByAOT);           \
    }                                                                                                                \
    Bind(&clearMachineCode);                                                                                         \
    {                                                                                                                \
        Store(VariableType::NATIVE_POINTER(), glue, *varOsrCache, *varMachineCodeOffset, Undefined());               \
        Jump(&executeBCByInterpreter);                                                                               \
    }                                                                                                                \
    Bind(&executeBCByAOT);                                                                                           \
    {                                                                                                                \
        DEFVARIABLE(varRetVal, VariableType::JS_ANY(), Undefined());                                                 \
        Label fastCallOptimized(env);                                                                                \
        Label callOptimized(env);                                                                                    \
        Label handleReturn(env);                                                                                     \
        Label resumeRspAndReturn(env);                                                                               \
        Label resumeRspAndDispatch(env);                                                                             \
        Store(VariableType::NATIVE_POINTER(), glue, curFunction, IntPtr(JSFunction::MACHINECODE_OFFSET),             \
              *varMachineCode);                                                                                      \
        GateRef execCnt =                                                                                            \
            LoadPrimitive(VariableType::INT16(), *varMachineCode, IntPtr(MachineCode::OSR_EXECUTE_CNT_OFFSET));      \
        GateRef newExecCnt = Int16Add(execCnt, Int16(1));                                                            \
        Store(VariableType::INT16(), glue, *varMachineCode, IntPtr(MachineCode::OSR_EXECUTE_CNT_OFFSET),             \
              newExecCnt);                                                                                           \
        GateRef codeAddr = LoadPrimitive(VariableType::NATIVE_POINTER(), *varMachineCode,                            \
                                IntPtr(MachineCode::FUNCADDR_OFFSET));                                               \
        varRetVal = FastCallOptimized(glue, codeAddr, { glue, sp });                                                 \
        Jump(&handleReturn);                                                                                         \
        Bind(&handleReturn);                                                                                         \
        {                                                                                                            \
            GateRef prevSp = LoadPrimitive(VariableType::NATIVE_POINTER(), curFrame,                                 \
                             IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));                        \
            GateRef prevFrame = GetFrame(prevSp);                                                                    \
            GateRef prevPc = GetPcFromFrame(prevFrame);                                                              \
            Branch(IntPtrEqual(prevPc, IntPtr(0)), &resumeRspAndReturn, &resumeRspAndDispatch);                      \
            Bind(&resumeRspAndReturn);                                                                               \
            {                                                                                                        \
                CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varRetVal, prevSp, sp});                      \
                Return();                                                                                            \
            }                                                                                                        \
            Bind(&resumeRspAndDispatch);                                                                             \
            {                                                                                                        \
                GateRef prevFunction = GetFunctionFromFrame(glue, prevFrame);                                        \
                GateRef prevMethod = Load(VariableType::JS_ANY(), glue, prevFunction,                                \
                                          IntPtr(JSFunctionBase::METHOD_OFFSET));                                    \
                GateRef prevConstpool = GetConstpoolFromMethod(glue, prevMethod);                                    \
                GateRef prevProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, prevFunction);                    \
                GateRef prevHotnessCounter = GetHotnessCounterFromMethod(prevMethod);                                \
                GateRef jumpSize = GetCallSizeFromFrame(prevFrame);                                                  \
                CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),                                                \
                               {glue, sp, prevPc, prevConstpool, prevProfileTypeInfo, *varRetVal,                    \
                                prevHotnessCounter, jumpSize});                                                      \
                Return();                                                                                            \
            }                                                                                                        \
        }                                                                                                            \
    }                                                                                                                \
    Bind(&executeBCByInterpreter)

#define DEFINE_BY_NAME(newIc)                                                                                        \
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));                                                               \
    GateRef stringId = ReadInst16_1(pc);                                                                             \
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));                           \
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));                                       \
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
    BRANCH(IsEcmaObject(glue, receiver), &isEcmaObj, &slowPath);                                                     \
    Bind(&isEcmaObj);                                                                                                \
    BRANCH(TaggedIsUndefined(profileTypeInfo), &hclassNotHit, &tryGetHclass);                                        \
    Bind(&tryGetHclass);                                                                                             \
    {                                                                                                                \
        GateRef firstValue = GetValueFromTaggedArray(glue, profileTypeInfo, slotId);                                 \
        BRANCH(TaggedIsHeapObject(firstValue), &firstValueHeapObject, &hclassNotHit);                                \
        Bind(&firstValueHeapObject);                                                                                 \
        GateRef hclass = LoadHClass(glue, *holder);                                                                  \
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
        GateRef hclass = LoadHClass(glue, *holder);                                                                  \
        GateRef jsType = GetObjectType(hclass);                                                                      \
        Label findProperty(env);                                                                                     \
        BRANCH(IsSpecialIndexedObj(jsType), &slowPath, &findProperty);                                               \
        Bind(&findProperty);                                                                                         \
        Label isDicMode(env);                                                                                        \
        Label notDicMode(env);                                                                                       \
        BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);                                           \
        Bind(&isDicMode);                                                                                            \
        {                                                                                                            \
            GateRef array = GetPropertiesArray(glue, *holder);                                                       \
            GateRef entry = FindEntryFromHashTable<NameDictionary>(glue, array, propKey);                            \
            BRANCH(Int32NotEqual(entry, Int32(-1)), &slowPath, &loopExit);                                           \
        }                                                                                                            \
        Bind(&notDicMode);                                                                                           \
        {                                                                                                            \
            GateRef layOutInfo = GetLayoutFromHClass(glue, hclass);                                                  \
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);                                                   \
            GateRef entry = FindElementWithCache(glue, layOutInfo, hclass, propKey, propsNum);                       \
            BRANCH(Int32NotEqual(entry, Int32(-1)), &slowPath, &loopExit);                                           \
        }                                                                                                            \
        Bind(&loopExit);                                                                                             \
        {                                                                                                            \
            holder = GetPrototypeFromHClass(glue, LoadHClass(glue, *holder));                                        \
            BRANCH(TaggedIsHeapObject(*holder), &loopEnd, &whichPath);                                               \
        }                                                                                                            \
        Bind(&loopEnd);                                                                                              \
        LoopEndWithCheckSafePoint(&loopHead, env, glue);                                                             \
    }                                                                                                                \
    Bind(&whichPath);                                                                                                \
    {                                                                                                                \
        BRANCH(newIc, &icPath, &slowPath);                                                                           \
    }                                                                                                                \
    Bind(&icPath);                                                                                                   \
    {                                                                                                                \
        /* IC do the same thing as stobjbyname */                                                                    \
        AccessObjectStubBuilder builder(this);                                                                       \
        StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_1, StringIdInfo::Length::BITS_16);               \
        result = builder.StOwnICByName(glue, receiver, 0, info, acc, profileTypeInfo, slotId, callback);             \
        Jump(&exit);                                                                                                 \
    }                                                                                                                \
    Bind(&slowPath);                                                                                                 \
    {                                                                                                                \
        result = DefineField(glue, receiver, propKey, acc);                                                          \
        Jump(&exit);                                                                                                 \
    }                                                                                                                \
    Bind(&exit)

template <bool needPrint>
void InterpreterStubBuilder::DebugPrintInstruction()
{
#if ECMASCRIPT_ENABLE_INTERPRETER_LOG
    if constexpr (!needPrint) {
        return;
    }
    GateRef glue = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::GLUE));
    GateRef pc = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::PC));
    CallNGCRuntime(glue, RTSTUB_ID(DebugPrintInstruction), { glue, pc });
#endif
#if ECMASCRIPT_ENABLE_COLLECTING_OPCODES
    GateRef opcodeGlue = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::GLUE));
    GateRef opcodePc = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::PC));
    CallNGCRuntime(opcodeGlue, RTSTUB_ID(CollectingOpcodes), { opcodeGlue, opcodePc });
#endif
}

DECLARE_ASM_HANDLER(HandleLdnan)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = DoubleToTaggedDoublePtr(Double(base::NAN_VALUE));
    DISPATCH_WITH_ACC(LDNAN);
}

DECLARE_ASM_HANDLER(HandleLdinfinity)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = DoubleToTaggedDoublePtr(Double(base::POSITIVE_INFINITY));
    DISPATCH_WITH_ACC(LDINFINITY);
}

DECLARE_ASM_HANDLER(HandleLdundefined)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = Undefined();
    DISPATCH_WITH_ACC(LDUNDEFINED);
}

DECLARE_ASM_HANDLER(HandleLdnull)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = Null();
    DISPATCH_WITH_ACC(LDNULL);
}

DECLARE_ASM_HANDLER(HandleLdsymbol)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = CallRuntime(glue, RTSTUB_ID(GetSymbolFunction), {});
    DISPATCH_WITH_ACC(LDSYMBOL);
}

DECLARE_ASM_HANDLER(HandleLdglobal)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = GetGlobalObject(glue);
    DISPATCH_WITH_ACC(LDGLOBAL);
}

DECLARE_ASM_HANDLER(HandleLdtrue)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = TaggedTrue();
    DISPATCH_WITH_ACC(LDTRUE);
}

DECLARE_ASM_HANDLER(HandleLdfalse)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = TaggedFalse();
    DISPATCH_WITH_ACC(LDFALSE);
}

DECLARE_ASM_HANDLER(HandleTypeofImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastTypeOf(glue, acc);
    DISPATCH_WITH_ACC(TYPEOF_IMM8);
}

DECLARE_ASM_HANDLER(HandleTypeofImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastTypeOf(glue, acc);
    DISPATCH_WITH_ACC(TYPEOF_IMM16);
}

DECLARE_ASM_HANDLER(HandlePoplexenv)
{
    GateRef state = GetFrame(sp);
    GateRef currentLexEnv = GetEnvFromFrame(glue, state);
    GateRef parentLexEnv = GetParentEnv(glue, currentLexEnv);
    SetEnvToFrame(glue, state, parentLexEnv);
    DISPATCH(POPLEXENV);
}

DECLARE_ASM_HANDLER(HandleDeprecatedPoplexenvPrefNone)
{
    GateRef state = GetFrame(sp);
    GateRef currentLexEnv = GetEnvFromFrame(glue, state);
    GateRef parentLexEnv = GetParentEnv(glue, currentLexEnv);
    SetEnvToFrame(glue, state, parentLexEnv);
    DISPATCH(DEPRECATED_POPLEXENV_PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleGetunmappedargs)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(argumentsList, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(argumentsObj, VariableType::JS_ANY(), Hole());
    auto env = GetEnvironment();
    GateRef startIdxAndNumArgs = GetStartIdxAndNumArgs(glue, sp, Int32(0));
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
    BRANCH(TaggedIsException(*argumentsList), &slowPath, &newArgumentsObj);
    Bind(&newArgumentsObj);
    Label afterArgumentsObj(env);
    newBuilder.NewArgumentsObj(&argumentsObj, &afterArgumentsObj, *argumentsList, numArgs);
    Bind(&afterArgumentsObj);
    BRANCH(TaggedIsException(*argumentsObj), &slowPath, &checkException);
    Bind(&checkException);
    BRANCH(HasPendingException(glue), &slowPath, &dispatch);
    Bind(&dispatch);
    {
        varAcc = *argumentsObj;
        DISPATCH_WITH_ACC(GETUNMAPPEDARGS);
    }

    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(GetUnmapedArgs), {});
        CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(GETUNMAPPEDARGS));
    }
}

DECLARE_ASM_HANDLER(HandleCopyrestargsImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    auto env = GetEnvironment();
    GateRef restIdx = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef startIdxAndNumArgs = GetStartIdxAndNumArgs(glue, sp, restIdx);
    GateRef startIdx = TruncInt64ToInt32(Int64LSR(startIdxAndNumArgs, Int64(32)));
    GateRef numArgs = TruncInt64ToInt32(startIdxAndNumArgs);
    Label dispatch(env);
    Label slowPath(env);
    // For the args, we use ElementsKind::GENERIC as the kind
    GateRef globalEnv = GetGlobalEnv(glue);
    GateRef intialHClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                             static_cast<size_t>(GlobalEnvField::ELEMENT_HOLE_TAGGED_HCLASS_INDEX));
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    res = newBuilder.NewJSArrayWithSize(intialHClass, numArgs);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, *res, lengthOffset, TruncInt64ToInt32(numArgs));
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    Store(VariableType::JS_ANY(), glue, *res,
          IntPtr(JSArray::GetInlinedPropertyOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX)), accessor);
    SetExtensibleToBitfield(glue, *res, true);
    Label setArgumentsBegin(env);
    Label setArgumentsAgain(env);
    Label setArgumentsEnd(env);
    GateRef elements = GetElementsArray(glue, *res);
    BRANCH(Int32UnsignedLessThan(*i, numArgs), &setArgumentsBegin, &setArgumentsEnd);
    LoopBegin(&setArgumentsBegin);
    {
        GateRef idx = ZExtInt32ToPtr(Int32Add(startIdx, *i));
        GateRef receiver = Load(VariableType::JS_ANY(), glue, sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, *i, receiver);
        i = Int32Add(*i, Int32(1));
        BRANCH(Int32UnsignedLessThan(*i, numArgs), &setArgumentsAgain, &setArgumentsEnd);
        Bind(&setArgumentsAgain);
    }
    LoopEnd(&setArgumentsBegin);
    Bind(&setArgumentsEnd);
    BRANCH(HasPendingException(glue), &slowPath, &dispatch);
    Bind(&dispatch);
    {
        varAcc = *res;
        DISPATCH_WITH_ACC(COPYRESTARGS_IMM8);
    }

    Bind(&slowPath);
    {
        GateRef result2 = CallRuntime(glue, RTSTUB_ID(CopyRestArgs), { IntToTaggedInt(restIdx) });
        CHECK_EXCEPTION_WITH_ACC(result2, INT_PTR(COPYRESTARGS_IMM8));
    }
}

DECLARE_ASM_HANDLER(HandleWideCopyrestargsPrefImm16)
{
    GateRef restIdx = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CopyRestArgs), { IntToTaggedInt(restIdx) });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(WIDE_COPYRESTARGS_PREF_IMM16));
}

DECLARE_ASM_HANDLER(HandleCreateobjectwithexcludedkeysImm8V8V8)
{
    GateRef numKeys = ReadInst8_0(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef firstArgRegIdx = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithExcludedKeys),
        { Int16ToTaggedInt(numKeys), obj, Int16ToTaggedInt(firstArgRegIdx) });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleWideCreateobjectwithexcludedkeysPrefImm16V8V8)
{
    GateRef numKeys = ReadInst16_1(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef firstArgRegIdx = ZExtInt8ToInt16(ReadInst8_4(pc));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithExcludedKeys),
        { Int16ToTaggedInt(numKeys), obj, Int16ToTaggedInt(firstArgRegIdx) });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8));
}

DECLARE_ASM_HANDLER(HandleThrowIfsupernotcorrectcallPrefImm16)
{
    GateRef imm = ReadInst16_1(pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        { Int16ToTaggedInt(imm), acc }); // acc is thisValue
    CHECK_EXCEPTION(res, INT_PTR(THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16));
}

DECLARE_ASM_HANDLER(HandleThrowIfsupernotcorrectcallPrefImm8)
{
    GateRef imm = ReadInst8_1(pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        { Int8ToTaggedInt(imm), acc }); // acc is thisValue
    CHECK_EXCEPTION(res, INT_PTR(THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8));
}

DECLARE_ASM_HANDLER(HandleAsyncfunctionresolveV8)
{
    GateRef asyncFuncObj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_0(pc)));
    GateRef value = acc;
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedTrue() });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(ASYNCFUNCTIONRESOLVE_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedAsyncfunctionresolvePrefV8V8V8)
{
    GateRef asyncFuncObj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedTrue() });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleAsyncfunctionrejectV8)
{
    GateRef asyncFuncObj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_0(pc)));
    GateRef value = acc;
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedFalse() });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(ASYNCFUNCTIONREJECT_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedAsyncfunctionrejectPrefV8V8V8)
{
    GateRef asyncFuncObj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedFalse() });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleDefinegettersetterbyvalueV8V8V8V8)
{
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_0(pc)));
    GateRef prop = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef getter = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef setter = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef frame = GetFrame(sp);
    GateRef func = GetFunctionFromFrame(glue, frame);

    GateRef method = Load(VariableType::JS_ANY(), glue, func, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef firstPC =
        LoadPrimitive(VariableType::NATIVE_POINTER(), method, IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    GateRef offset = TaggedPtrToTaggedIntPtr(PtrSub(pc, firstPC));
    GateRef res = CallRuntime(glue, RTSTUB_ID(DefineGetterSetterByValue),
                              { obj, prop, getter, setter, acc, func, offset }); // acc is flag
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleGetpropiterator)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.EnumerateObjectProperties(glue, *varAcc);
    CHECK_EXCEPTION_WITH_VARACC(res, INT_PTR(GETPROPITERATOR));
}

DECLARE_ASM_HANDLER(HandleAsyncfunctionenter)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionEnter), {});
    CHECK_EXCEPTION_WITH_VARACC(res, INT_PTR(ASYNCFUNCTIONENTER));
}

DECLARE_ASM_HANDLER(HandleLdhole)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = Hole();
    DISPATCH_WITH_ACC(LDHOLE);
}

DECLARE_ASM_HANDLER(HandleCreateemptyobject)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    NewObjectStubBuilder newBuilder(this);
    varAcc = newBuilder.CreateEmptyObject(glue);
    DISPATCH_WITH_ACC(CREATEEMPTYOBJECT);
}

DECLARE_ASM_HANDLER(HandleCreateemptyarrayImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    NewObjectStubBuilder newBuilder(this);
    GateRef frame = GetFrame(*varSp);
    GateRef func = GetFunctionFromFrame(glue, frame);
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    varAcc = newBuilder.CreateEmptyArray(glue, func, { pc, 0, true }, profileTypeInfo, slotId, callback);
    DISPATCH_WITH_ACC(CREATEEMPTYARRAY_IMM8);
}

DECLARE_ASM_HANDLER(HandleCreateemptyarrayImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    NewObjectStubBuilder newBuilder(this);
    GateRef frame = GetFrame(*varSp);
    GateRef func = GetFunctionFromFrame(glue, frame);
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    varAcc = newBuilder.CreateEmptyArray(glue, func, { pc, 0 , true }, profileTypeInfo, slotId, callback);
    DISPATCH_WITH_ACC(CREATEEMPTYARRAY_IMM16);
}

DECLARE_ASM_HANDLER(HandleGetiteratorImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = GetIterator(glue, *varAcc, callback);
    CHECK_PENDING_EXCEPTION(res, INT_PTR(GETITERATOR_IMM8));
}

DECLARE_ASM_HANDLER(HandleGetiteratorImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = GetIterator(glue, *varAcc, callback);
    CHECK_PENDING_EXCEPTION(res, INT_PTR(GETITERATOR_IMM16));
}

DECLARE_ASM_HANDLER(HandleGetasynciteratorImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(GetAsyncIterator), { *varAcc });
    CHECK_PENDING_EXCEPTION(res, INT_PTR(GETASYNCITERATOR_IMM8));
}

DECLARE_ASM_HANDLER(HandleLdPrivatePropertyImm8Imm16Imm16)
{
    GateRef lexicalEnv = GetEnvFromFrame(glue, GetFrame(sp));
    GateRef levelIndex = ReadInst16_1(pc);
    GateRef slotIndex = ReadInst16_3(pc);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = CallRuntime(glue,
                         RTSTUB_ID(LdPrivateProperty),
                         {lexicalEnv, IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), acc}); // acc as obj
    CHECK_EXCEPTION_WITH_ACC(*result, INT_PTR(LDPRIVATEPROPERTY_IMM8_IMM16_IMM16));
}

DECLARE_ASM_HANDLER(HandleStPrivatePropertyImm8Imm16Imm16V8)
{
    GateRef lexicalEnv = GetEnvFromFrame(glue, GetFrame(sp));
    GateRef levelIndex = ReadInst16_1(pc);
    GateRef slotIndex = ReadInst16_3(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result =
        CallRuntime(glue,
                    RTSTUB_ID(StPrivateProperty),
                    {lexicalEnv, IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), obj, acc}); // acc as value
    CHECK_EXCEPTION_WITH_ACC(*result, INT_PTR(STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8));
}

DECLARE_ASM_HANDLER(HandleTestInImm8Imm16Imm16)
{
    GateRef lexicalEnv = GetEnvFromFrame(glue, GetFrame(sp));
    [[maybe_unused]] GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef levelIndex = ReadInst16_1(pc);
    GateRef slotIndex = ReadInst16_3(pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(TestIn), {lexicalEnv,
        IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), acc});  // acc as obj
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(TESTIN_IMM8_IMM16_IMM16));
}

DECLARE_ASM_HANDLER(HandleThrowPatternnoncoerciblePrefNone)
{
    CallRuntime(glue, RTSTUB_ID(ThrowPatternNonCoercible), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleThrowDeletesuperpropertyPrefNone)
{
    CallRuntime(glue, RTSTUB_ID(ThrowDeleteSuperProperty), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleDebugger)
{
    CallRuntime(glue, RTSTUB_ID(NotifyDebuggerStatement), {});
    DISPATCH(DEBUGGER);
}

DECLARE_ASM_HANDLER(HandleMul2Imm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Mul(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(MUL2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleDiv2Imm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Div(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DIV2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleMod2Imm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Mod(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(MOD2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleEqImm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Equal(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(EQ_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleJequndefinedImm16)
{
    DISPATCH(NOP);
}

DECLARE_ASM_HANDLER(HandleJequndefinedImm8)
{
    DISPATCH(NOP);
}

DECLARE_ASM_HANDLER(HandleNoteqImm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.NotEqual(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(NOTEQ_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleLessImm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Less(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(LESS_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleLesseqImm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.LessEq(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(LESSEQ_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleGreaterImm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Greater(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(GREATER_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleGreatereqImm8V8)
{
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.GreaterEq(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(GREATEREQ_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleNop)
{
    DISPATCH(NOP);
}

DECLARE_ASM_HANDLER(HandleLdaV8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef vsrc = ReadInst8_0(pc);
    varAcc = GetVregValue(glue, sp, ZExtInt8ToPtr(vsrc));
    DISPATCH_WITH_ACC(LDA_V8);
}

DECLARE_ASM_HANDLER(HandleStaV8)
{
    GateRef vdst = ReadInst8_0(pc);
    SetVregValue(glue, sp, ZExtInt8ToPtr(vdst), acc);
    DISPATCH(STA_V8);
}

DECLARE_ASM_HANDLER(HandleJmpImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    GateRef offset = ReadInstSigned8_0(pc);
    Label dispatch(env);
    Label slowPath(env);

    TRY_OSR();

    UPDATE_HOTNESS(sp, callback);
    DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleJmpImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    GateRef offset = ReadInstSigned16_0(pc);
    Label dispatch(env);
    Label slowPath(env);

    TRY_OSR();

    UPDATE_HOTNESS(sp, callback);
    DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleJmpImm32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    GateRef offset = ReadInstSigned32_0(pc);
    Label dispatch(env);
    Label slowPath(env);

    TRY_OSR();

    UPDATE_HOTNESS(sp, callback);
    DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleLdlexvarImm4Imm4)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt8ToInt32(ReadInst4_0(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_1(pc));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(glue, *currentEnv, slot);
    varAcc = variable;

    DISPATCH_WITH_ACC(LDLEXVAR_IMM4_IMM4);
}

DECLARE_ASM_HANDLER(HandleLdlexvarImm8Imm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_1(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(glue, *currentEnv, slot);
    varAcc = variable;
    DISPATCH_WITH_ACC(LDLEXVAR_IMM8_IMM8);
}

DECLARE_ASM_HANDLER(HandleWideLdlexvarPrefImm16Imm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(glue, *currentEnv, slot);
    varAcc = variable;
    DISPATCH_WITH_ACC(WIDE_LDLEXVAR_PREF_IMM16_IMM16);
}

DECLARE_ASM_HANDLER(HandleStlexvarImm4Imm4)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt8ToInt32(ReadInst4_0(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_1(pc));

    GateRef value = acc;
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(STLEXVAR_IMM4_IMM4);
}

DECLARE_ASM_HANDLER(HandleStlexvarImm8Imm8)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_1(pc));

    GateRef value = acc;
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(STLEXVAR_IMM8_IMM8);
}

DECLARE_ASM_HANDLER(HandleWideStlexvarPrefImm16Imm16)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));

    GateRef value = acc;
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(WIDE_STLEXVAR_PREF_IMM16_IMM16);
}

DECLARE_ASM_HANDLER(HandleDeprecatedStlexvarPrefImm16Imm16V8)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));

    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8);
}

DECLARE_ASM_HANDLER(HandleDeprecatedStlexvarPrefImm8Imm8V8)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_2(pc));

    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8);
}

DECLARE_ASM_HANDLER(HandleDeprecatedStlexvarPrefImm4Imm4V8)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt8ToInt32(ReadInst4_2(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_3(pc));

    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(glue, state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8);
}

DECLARE_ASM_HANDLER(HandleIncImm8)
{
    OperationsStubBuilder builder(this);
    GateRef result = builder.Inc(glue, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(INC_IMM8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedIncPrefV8)
{
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Inc(glue, value);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_INC_PREF_V8));
}

DECLARE_ASM_HANDLER(HandleDecImm8)
{
    OperationsStubBuilder builder(this);
    GateRef result = builder.Dec(glue, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEC_IMM8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedDecPrefV8)
{
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Dec(glue, value);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_DEC_PREF_V8));
}

DECLARE_ASM_HANDLER(HandleExpImm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef base = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(Exp), { base, acc });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(EXP_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleIsinImm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef prop = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
#if ENABLE_NEXT_OPTIMIZATION
    GateRef result = IsIn(glue, prop, acc); // acc is obj
#else
    GateRef result = CallRuntime(glue, RTSTUB_ID(IsIn), {prop, acc}); // acc is obj
#endif
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(ISIN_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleInstanceofImm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef target = acc;
    AccessObjectStubBuilder builder(this);
    GateRef result = InstanceOf(glue, obj, target, profileTypeInfo, slotId, callback);
    CHECK_PENDING_EXCEPTION(result, INT_PTR(INSTANCEOF_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleStrictnoteqImm8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));

    OperationsStubBuilder builder(this);
    varAcc = builder.StrictNotEqual(glue, left, acc, callback);
    DISPATCH_WITH_ACC(STRICTNOTEQ_IMM8_V8);
}

DECLARE_ASM_HANDLER(HandleStricteqImm8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));

    OperationsStubBuilder builder(this);
    varAcc = builder.StrictEqual(glue, left, acc, callback);
    DISPATCH_WITH_ACC(STRICTEQ_IMM8_V8);
}

DECLARE_ASM_HANDLER(HandleResumegenerator)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef obj = *varAcc;

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef frame = GetFrame(sp);
    GateRef curFunc = GetFunctionFromFrame(glue, frame);
    CallNGCRuntime(glue, RTSTUB_ID(StartCallTimer), { glue, curFunc, False() });
#endif
    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    BRANCH(TaggedIsAsyncGeneratorObject(glue, obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    Bind(&isAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), glue, obj, resumeResultOffset);
        Jump(&dispatch);
    }
    Bind(&notAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), glue, obj, resumeResultOffset);
        GateRef taskInfoOffset = IntPtr(JSGeneratorObject::TASK_INFO_OFFSET);
        GateRef taskInfo = LoadPrimitive(VariableType::NATIVE_POINTER(), obj, taskInfoOffset);
        Store(VariableType::NATIVE_POINTER(), glue, glue,
            IntPtr(JSThread::GlueData::GetTaskInfoOffset(env->Is32Bit())), taskInfo);
        Store(VariableType::NATIVE_POINTER(), glue, obj,
            IntPtr(JSGeneratorObject::TASK_INFO_OFFSET), IntPtr(0));
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(RESUMEGENERATOR);
}

DECLARE_ASM_HANDLER(HandleDeprecatedResumegeneratorPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef frame = GetFrame(sp);
    GateRef curFunc = GetFunctionFromFrame(glue, frame);
    CallNGCRuntime(glue, RTSTUB_ID(StartCallTimer), { glue, curFunc, False() });
#endif

    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    BRANCH(TaggedIsAsyncGeneratorObject(glue, obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    Bind(&isAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), glue, obj, resumeResultOffset);
        Jump(&dispatch);
    }
    Bind(&notAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = IntPtr(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        varAcc = Load(VariableType::JS_ANY(), glue, obj, resumeResultOffset);
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(DEPRECATED_RESUMEGENERATOR_PREF_V8);
}

DECLARE_ASM_HANDLER(HandleGetresumemode)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef obj = *varAcc;

    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    BRANCH(TaggedIsAsyncGeneratorObject(glue, obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
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
    DISPATCH_WITH_ACC(GETRESUMEMODE);
}

DECLARE_ASM_HANDLER(HandleDeprecatedGetresumemodePrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));

    Label isAsyncGeneratorObj(env);
    Label notAsyncGeneratorObj(env);
    Label dispatch(env);
    BRANCH(TaggedIsAsyncGeneratorObject(glue, obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
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
    DISPATCH_WITH_ACC(DEPRECATED_GETRESUMEMODE_PREF_V8);
}

DECLARE_ASM_HANDLER(HandleCreategeneratorobjV8)
{
    GateRef v0 = ReadInst8_0(pc);
    GateRef genFunc = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateGeneratorObj), { genFunc });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(CREATEGENERATOROBJ_V8));
}

DECLARE_ASM_HANDLER(HandleThrowConstassignmentPrefV8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    CallRuntime(glue, RTSTUB_ID(ThrowConstAssignment), { value });
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleGettemplateobjectImm8)
{
    GateRef literal = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(GETTEMPLATEOBJECT_IMM8));
}

DECLARE_ASM_HANDLER(HandleGettemplateobjectImm16)
{
    GateRef literal = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(GETTEMPLATEOBJECT_IMM16));
}

DECLARE_ASM_HANDLER(HandleDeprecatedGettemplateobjectPrefV8)
{
    GateRef literal = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_GETTEMPLATEOBJECT_PREF_V8));
}

DECLARE_ASM_HANDLER(HandleGetnextpropnameV8)
{
    GateRef v0 = ReadInst8_0(pc);
    GateRef iter = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef result = NextInternal(glue, iter);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(GETNEXTPROPNAME_V8));
}

DECLARE_ASM_HANDLER(HandleThrowIfnotobjectPrefV8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    Label isEcmaObject(env);
    Label notEcmaObject(env);
    Label isHeapObject(env);
    BRANCH(TaggedIsHeapObject(value), &isHeapObject, &notEcmaObject);
    Bind(&isHeapObject);
    BRANCH(TaggedObjectIsEcmaObject(glue, value), &isEcmaObject, &notEcmaObject);
    Bind(&isEcmaObject);
    {
        DISPATCH(THROW_IFNOTOBJECT_PREF_V8);
    }
    Bind(&notEcmaObject);
    CallRuntime(glue, RTSTUB_ID(ThrowIfNotObject), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleCloseiteratorImm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(CloseIterator), { iter });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(CLOSEITERATOR_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleCloseiteratorImm16V8)
{
    GateRef v0 = ReadInst8_2(pc);
    GateRef iter = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(CloseIterator), { iter });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(CLOSEITERATOR_IMM16_V8));
}

DECLARE_ASM_HANDLER(HandleSupercallspreadImm8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef v0 = ReadInst8_1(pc);
    GateRef array = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef thisFunc = acc;
    GateRef newTarget = GetNewTarget(glue, sp);
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
    Label noException(env);

    BRANCH(TaggedIsHeapObject(superCtor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    BRANCH(IsJSFunction(glue, superCtor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    BRANCH(IsConstructor(glue, superCtor), &ctorIsConstructor, &slowPath);
    Bind(&ctorIsConstructor);
    BRANCH(TaggedIsUndefined(newTarget), &slowPath, &normalPath);
    Bind(&normalPath);
    {
        BRANCH(IsBase(glue, superCtor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        NewObjectStubBuilder objBuilder(this);
        thisObj = objBuilder.FastSuperAllocateThis(glue, superCtor, newTarget);
        BRANCH(HasPendingException(glue), &isException, &ctorNotBase);
        Bind(&ctorNotBase);
        GateRef argvLen = LoadPrimitive(VariableType::INT32(), array, IntPtr(JSArray::LENGTH_OFFSET));
        GateRef srcElements = GetCallSpreadArgs(glue, array, callback);
        BRANCH(TaggedIsException(srcElements), &isException, &noException);
        Bind(&noException);
        GateRef jumpSize = IntPtr(InterpreterAssembly::GetCallSize(EcmaOpcode::SUPERCALLSPREAD_IMM8_V8));
        METHOD_ENTRY_ENV_DEFINED(superCtor);
        GateRef elementsPtr = PtrAdd(srcElements, IntPtr(TaggedArray::DATA_OFFSET));
        JSCallArgs callArgs(JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV);
        callArgs.superCallArgs = { thisFunc, array, ZExtInt32ToPtr(argvLen), elementsPtr, *thisObj, newTarget };
        CallStubBuilder callBuilder(this, glue, superCtor, argvLen, jumpSize, nullptr, hotnessCounter, callArgs,
            callback);
        res = callBuilder.JSCallDispatch();
        Jump(&threadCheck);
    }
    Bind(&slowPath);
    {
        res = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), { thisFunc, array });
        Jump(&threadCheck);
    }
    Bind(&threadCheck);
    {
        GateRef resVal = *res;
        GateRef isError = LogicAndBuilder(env).And(TaggedIsException(resVal)).And(HasPendingException(glue)).Done();
        BRANCH(isError, &isException, &dispatch);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&dispatch);
    {
        varAcc = *res;
        DISPATCH_WITH_ACC(SUPERCALLSPREAD_IMM8_V8);
    }
}

DECLARE_ASM_HANDLER(HandleDelobjpropV8)
{
    GateRef v0 = ReadInst8_0(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef prop = acc;
    GateRef result = DeletePropertyOrThrow(glue, obj, prop);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DELOBJPROP_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedDelobjpropPrefV8V8)
{
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef prop = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef result = CallRuntime(glue, RTSTUB_ID(DelObjProp), { obj, prop });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_DELOBJPROP_PREF_V8_V8));
}

DECLARE_ASM_HANDLER(HandleNewobjapplyImm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef func = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(NewObjApply), { func, acc }); // acc is array
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(NEWOBJAPPLY_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleNewobjapplyImm16V8)
{
    GateRef v0 = ReadInst8_2(pc);
    GateRef func = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(NewObjApply), { func, acc }); // acc is array
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(NEWOBJAPPLY_IMM16_V8));
}

DECLARE_ASM_HANDLER(HandleCreateiterresultobjV8V8)
{
    GateRef v0 = ReadInst8_0(pc);
    GateRef v1 = ReadInst8_1(pc);
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef flag = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateIterResultObj), { value, flag });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(CREATEITERRESULTOBJ_V8_V8));
}

DECLARE_ASM_HANDLER(HandleAsyncfunctionawaituncaughtV8)
{
    GateRef v0 = ReadInst8_0(pc);
    GateRef asyncFuncObj = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef value = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncFunctionAwaitUncaught), { asyncFuncObj, value });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(ASYNCFUNCTIONAWAITUNCAUGHT_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedAsyncfunctionawaituncaughtPrefV8V8)
{
    GateRef asyncFuncObj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncFunctionAwaitUncaught), { asyncFuncObj, value });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8));
}

DECLARE_ASM_HANDLER(HandleThrowUndefinedifholePrefV8V8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef hole = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    Label isHole(env);
    Label notHole(env);
    BRANCH(TaggedIsHole(hole), &isHole, &notHole);
    Bind(&notHole);
    {
        DISPATCH(THROW_UNDEFINEDIFHOLE_PREF_V8_V8);
    }
    Bind(&isHole);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    // assert obj.IsString()
    CallRuntime(glue, RTSTUB_ID(ThrowUndefinedIfHole), { obj });
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleThrowUndefinedifholewithnamePrefId16)
{
    auto env = GetEnvironment();

    GateRef hole = acc;
    Label isHole(env);
    Label notHole(env);
    BRANCH(TaggedIsHole(hole), &isHole, &notHole);
    Bind(&notHole);
    {
        DISPATCH(THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16);
    }
    Bind(&isHole);
    GateRef stringId = ReadInst16_1(pc);
    GateRef str = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    // assert obj.IsString()
    CallRuntime(glue, RTSTUB_ID(ThrowUndefinedIfHole), { str });
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleCopydatapropertiesV8)
{
    GateRef v0 = ReadInst8_0(pc);
    GateRef dst = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef src = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(CopyDataProperties), { dst, src });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(COPYDATAPROPERTIES_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedCopydatapropertiesPrefV8V8)
{
    GateRef dst = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef src = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef result = CallRuntime(glue, RTSTUB_ID(CopyDataProperties), { dst, src });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8));
}

DECLARE_ASM_HANDLER(HandleStarrayspreadV8V8)
{
    GateRef v0 = ReadInst8_0(pc);
    GateRef v1 = ReadInst8_1(pc);
    GateRef dst = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef index = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StArraySpread), { dst, index, acc }); // acc is res
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(STARRAYSPREAD_V8_V8));
}

DECLARE_ASM_HANDLER(HandleSetobjectwithprotoImm8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef proto = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef obj = *varAcc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, obj });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    DISPATCH_WITH_ACC(SETOBJECTWITHPROTO_IMM8_V8);
}

DECLARE_ASM_HANDLER(HandleSetobjectwithprotoImm16V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_2(pc);
    GateRef proto = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef obj = *varAcc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, obj });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    DISPATCH_WITH_ACC(SETOBJECTWITHPROTO_IMM16_V8);
}

DECLARE_ASM_HANDLER(HandleDeprecatedSetobjectwithprotoPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef proto = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, obj });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    DISPATCH_WITH_ACC(DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleStobjbyvalueImm8V8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    GateRef value = acc;
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, value, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STOBJBYVALUE_IMM8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleStobjbyvalueImm16V8V8)
{
    GateRef v0 = ReadInst8_2(pc);
    GateRef v1 = ReadInst8_3(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    GateRef value = acc;
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, value, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STOBJBYVALUE_IMM16_V8_V8));
}

DECLARE_ASM_HANDLER(HandleStownbyvalueImm8V8V8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    Label isHeapObject(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    Label notClassConstructor(env);
    BRANCH(IsClassConstructor(glue, receiver), &slowPath, &notClassConstructor);
    Bind(&notClassConstructor);
    Label notClassPrototype(env);
    BRANCH(IsClassPrototype(glue, receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        GateRef result = SetPropertyByValue(glue, receiver, propKey, acc, true, callback); // acc is value
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION(result, INT_PTR(STOWNBYVALUE_IMM8_V8_V8));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByValue), { receiver, propKey, acc });
        CHECK_EXCEPTION(result, INT_PTR(STOWNBYVALUE_IMM8_V8_V8));
    }
}

DECLARE_ASM_HANDLER(HandleStownbyvalueImm16V8V8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_2(pc);
    GateRef v1 = ReadInst8_3(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    Label isHeapObject(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    Label notClassConstructor(env);
    BRANCH(IsClassConstructor(glue, receiver), &slowPath, &notClassConstructor);
    Bind(&notClassConstructor);
    Label notClassPrototype(env);
    BRANCH(IsClassPrototype(glue, receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        GateRef result = SetPropertyByValue(glue, receiver, propKey, acc, true, callback); // acc is value
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION(result, INT_PTR(STOWNBYVALUE_IMM16_V8_V8));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByValue), { receiver, propKey, acc });
        CHECK_EXCEPTION(result, INT_PTR(STOWNBYVALUE_IMM16_V8_V8));
    }
}

DECLARE_ASM_HANDLER(HandleStsuperbyvalueImm8V8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
     // acc is value, sp for thisFunc
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    CHECK_EXCEPTION(result, INT_PTR(STSUPERBYVALUE_IMM8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleStsuperbyvalueImm16V8V8)
{
    GateRef v0 = ReadInst8_2(pc);
    GateRef v1 = ReadInst8_3(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
     // acc is value, sp for thisFunc
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    CHECK_EXCEPTION(result, INT_PTR(STSUPERBYVALUE_IMM16_V8_V8));
}

DECLARE_ASM_HANDLER(HandleStsuperbynameImm8Id16V8)
{
    GateRef stringId = ReadInst16_1(pc);
    GateRef v0 = ReadInst8_3(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    CHECK_EXCEPTION(result, INT_PTR(STSUPERBYNAME_IMM8_ID16_V8));
}

DECLARE_ASM_HANDLER(HandleStsuperbynameImm16Id16V8)
{
    GateRef stringId = ReadInst16_2(pc);
    GateRef v0 = ReadInst8_4(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    CHECK_EXCEPTION(result, INT_PTR(STSUPERBYNAME_IMM16_ID16_V8));
}

DECLARE_ASM_HANDLER(HandleStobjbyindexImm8V8Imm16)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef index = ZExtInt16ToInt32(ReadInst16_2(pc));
    Label fastPath(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = SetPropertyByIndex(glue, receiver, index, acc, false);
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION(result, INT_PTR(STOBJBYINDEX_IMM8_V8_IMM16));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        CHECK_EXCEPTION(result, INT_PTR(STOBJBYINDEX_IMM8_V8_IMM16));
    }
}

DECLARE_ASM_HANDLER(HandleStobjbyindexImm16V8Imm16)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef index = ZExtInt16ToInt32(ReadInst16_3(pc));
    Label fastPath(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = SetPropertyByIndex(glue, receiver, index, acc, false);
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION(result, INT_PTR(STOBJBYINDEX_IMM16_V8_IMM16));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        CHECK_EXCEPTION(result, INT_PTR(STOBJBYINDEX_IMM16_V8_IMM16));
    }
}

DECLARE_ASM_HANDLER(HandleWideStobjbyindexPrefV8Imm32)
{
    auto env = GetEnvironment();

    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef index = ReadInst32_2(pc);
    Label fastPath(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = SetPropertyByIndex(glue, receiver, index, acc, false);
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION(result, INT_PTR(WIDE_STOBJBYINDEX_PREF_V8_IMM32));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        CHECK_EXCEPTION(result, INT_PTR(WIDE_STOBJBYINDEX_PREF_V8_IMM32));
    }
}

DECLARE_ASM_HANDLER(HandleStownbyindexImm8V8Imm16)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef index = ZExtInt16ToInt32(ReadInst16_2(pc));
    GateRef value = acc;
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreOwnByIndex(glue, receiver, index, value, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STOWNBYINDEX_IMM8_V8_IMM16));
}

DECLARE_ASM_HANDLER(HandleStownbyindexImm16V8Imm16)
{
    GateRef v0 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef index = ZExtInt16ToInt32(ReadInst16_3(pc));
    GateRef value = acc;
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreOwnByIndex(glue, receiver, index, value, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STOWNBYINDEX_IMM16_V8_IMM16));
}

DECLARE_ASM_HANDLER(HandleWideStownbyindexPrefV8Imm32)
{
    auto env = GetEnvironment();

    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef index = ReadInst32_2(pc);
    Label isHeapObject(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    Label notClassConstructor(env);
    BRANCH(IsClassConstructor(glue, receiver), &slowPath, &notClassConstructor);
    Bind(&notClassConstructor);
    Label notClassPrototype(env);
    BRANCH(IsClassPrototype(glue, receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        GateRef result = SetPropertyByIndex(glue, receiver, index, acc, true); // acc is value
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION(result, INT_PTR(WIDE_STOWNBYINDEX_PREF_V8_IMM32));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByIndex),
                                     { receiver, IntToTaggedInt(index), acc });
        CHECK_EXCEPTION(result, INT_PTR(WIDE_STOWNBYINDEX_PREF_V8_IMM32));
    }
}

DECLARE_ASM_HANDLER(HandleNegImm8)
{
    OperationsStubBuilder builder(this);
    GateRef result = builder.Neg(glue, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(NEG_IMM8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedNegPrefV8)
{
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Neg(glue, value);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_NEG_PREF_V8));
}

DECLARE_ASM_HANDLER(HandleNotImm8)
{
    OperationsStubBuilder builder(this);
    GateRef result = builder.Not(glue, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(NOT_IMM8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedNotPrefV8)
{
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Not(glue, value);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_NOT_PREF_V8));
}

DECLARE_ASM_HANDLER(HandleAnd2Imm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    OperationsStubBuilder builder(this);
    GateRef result = builder.And(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(AND2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleOr2Imm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Or(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(OR2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleXor2Imm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Xor(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(XOR2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleAshr2Imm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Ashr(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(ASHR2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleShr2Imm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Shr(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(SHR2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleShl2Imm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Shl(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(SHL2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleStobjbynameImm8Id16V8)
{
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_1, StringIdInfo::Length::BITS_16);
    GateRef result = builder.StoreObjByName(glue, receiver, 0, info, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STOBJBYNAME_IMM8_ID16_V8));
}

DECLARE_ASM_HANDLER(HandleStobjbynameImm16Id16V8)
{
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_4(pc)));
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));

    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_2, StringIdInfo::Length::BITS_16);
    GateRef result = builder.StoreObjByName(glue, receiver, 0, info, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STOBJBYNAME_IMM16_ID16_V8));
}

DECLARE_ASM_HANDLER(HandleStownbyvaluewithnamesetImm16V8V8)
{
    auto env = GetEnvironment();
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));

    Label isHeapObject(env);
    Label slowPath(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label notException(env);
    Label notException1(env);
    BRANCH(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    {
        BRANCH(IsClassConstructor(glue, receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            BRANCH(IsClassPrototype(glue, receiver), &slowPath, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByValue(glue, receiver, propKey, acc, true, callback, true);
                BRANCH(TaggedIsHole(res), &slowPath, &notHole);
                Bind(&notHole);
                {
                    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
                    Bind(&notException);
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    DISPATCH(STOWNBYVALUEWITHNAMESET_IMM16_V8_V8);
                }
            }
        }
    }
    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, acc });
        CHECK_EXCEPTION_WITH_JUMP(res, &notException1);
        Bind(&notException1);
        DISPATCH(STOWNBYVALUEWITHNAMESET_IMM16_V8_V8);
    }
}

DECLARE_ASM_HANDLER(HandleStownbyvaluewithnamesetImm8V8V8)
{
    auto env = GetEnvironment();
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));

    Label isHeapObject(env);
    Label slowPath(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label notException(env);
    Label notException1(env);
    BRANCH(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    {
        BRANCH(IsClassConstructor(glue, receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            BRANCH(IsClassPrototype(glue, receiver), &slowPath, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByValue(glue, receiver, propKey, acc, true, callback, true);
                BRANCH(TaggedIsHole(res), &slowPath, &notHole);
                Bind(&notHole);
                {
                    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
                    Bind(&notException);
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    DISPATCH(STOWNBYVALUEWITHNAMESET_IMM8_V8_V8);
                }
            }
        }
    }
    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, acc });
        CHECK_EXCEPTION_WITH_JUMP(res, &notException1);
        Bind(&notException1);
        DISPATCH(STOWNBYVALUEWITHNAMESET_IMM8_V8_V8);
    }
}

DECLARE_ASM_HANDLER(HandleStownbynameImm8Id16V8)
{
    auto env = GetEnvironment();
    GateRef stringId = ReadInst16_1(pc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label checkResult(env);

    Label isJSObject(env);
    Label slowPath(env);
    BRANCH(IsJSObject(glue, receiver), &isJSObject, &slowPath);
    Bind(&isJSObject);
    {
        Label notClassConstructor(env);
        BRANCH(IsClassConstructor(glue, receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Label fastPath(env);
            BRANCH(IsClassPrototype(glue, receiver), &slowPath, &fastPath);
            Bind(&fastPath);
            {
                result = SetPropertyByName(glue, receiver, propKey, acc, true, True(), callback);
                BRANCH(TaggedIsHole(*result), &slowPath, &checkResult);
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
        CHECK_EXCEPTION(*result, INT_PTR(STOWNBYNAME_IMM8_ID16_V8));
    }
}

DECLARE_ASM_HANDLER(HandleStownbynameImm16Id16V8)
{
    auto env = GetEnvironment();
    GateRef stringId = ReadInst16_2(pc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_4(pc)));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label checkResult(env);

    Label isJSObject(env);
    Label slowPath(env);
    BRANCH(IsJSObject(glue, receiver), &isJSObject, &slowPath);
    Bind(&isJSObject);
    {
        Label notClassConstructor(env);
        BRANCH(IsClassConstructor(glue, receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Label fastPath(env);
            BRANCH(IsClassPrototype(glue, receiver), &slowPath, &fastPath);
            Bind(&fastPath);
            {
                result = SetPropertyByName(glue, receiver, propKey, acc, true, True(), callback);
                BRANCH(TaggedIsHole(*result), &slowPath, &checkResult);
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
        CHECK_EXCEPTION(*result, INT_PTR(STOWNBYNAME_IMM16_ID16_V8));
    }
}

DECLARE_ASM_HANDLER(HandleStownbynamewithnamesetImm8Id16V8)
{
    auto env = GetEnvironment();
    GateRef stringId = ReadInst16_1(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    Label isJSObject(env);
    Label notJSObject(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label notException(env);
    Label notException1(env);
    BRANCH(IsJSObject(glue, receiver), &isJSObject, &notJSObject);
    Bind(&isJSObject);
    {
        BRANCH(IsClassConstructor(glue, receiver), &notJSObject, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            BRANCH(IsClassPrototype(glue, receiver), &notJSObject, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByName(glue, receiver, propKey, acc, true, True(), callback, false, true);
                BRANCH(TaggedIsHole(res), &notJSObject, &notHole);
                Bind(&notHole);
                {
                    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
                    Bind(&notException);
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    DISPATCH(STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8);
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, acc });
        CHECK_EXCEPTION_WITH_JUMP(res, &notException1);
        Bind(&notException1);
        DISPATCH(STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8);
    }
}
DECLARE_ASM_HANDLER(HandleStownbynamewithnamesetImm16Id16V8)
{
    auto env = GetEnvironment();
    GateRef stringId = ReadInst16_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_4(pc)));
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    Label isJSObject(env);
    Label notJSObject(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label notException(env);
    Label notException1(env);
    BRANCH(IsJSObject(glue, receiver), &isJSObject, &notJSObject);
    Bind(&isJSObject);
    {
        BRANCH(IsClassConstructor(glue, receiver), &notJSObject, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            BRANCH(IsClassPrototype(glue, receiver), &notJSObject, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByName(glue, receiver, propKey, acc, true, True(), callback, false, true);
                BRANCH(TaggedIsHole(res), &notJSObject, &notHole);
                Bind(&notHole);
                {
                    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
                    Bind(&notException);
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    DISPATCH(STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8);
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, acc });
        CHECK_EXCEPTION_WITH_JUMP(res, &notException1);
        Bind(&notException1);
        DISPATCH(STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8);
    }
}

DECLARE_ASM_HANDLER(HandleLdfunction)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    varAcc = GetFunctionFromFrame(glue, GetFrame(sp));
    DISPATCH_WITH_ACC(LDFUNCTION);
}

DECLARE_ASM_HANDLER(HandleMovV4V4)
{
    GateRef vdst = ZExtInt8ToPtr(ReadInst4_0(pc));
    GateRef vsrc = ZExtInt8ToPtr(ReadInst4_1(pc));
    GateRef value = GetVregValue(glue, sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    DISPATCH(MOV_V4_V4);
}

DECLARE_ASM_HANDLER(HandleMovV8V8)
{
    GateRef vdst = ZExtInt8ToPtr(ReadInst8_0(pc));
    GateRef vsrc = ZExtInt8ToPtr(ReadInst8_1(pc));
    GateRef value = GetVregValue(glue, sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    DISPATCH(MOV_V8_V8);
}

DECLARE_ASM_HANDLER(HandleMovV16V16)
{
    GateRef vdst = ZExtInt16ToPtr(ReadInst16_0(pc));
    GateRef vsrc = ZExtInt16ToPtr(ReadInst16_2(pc));
    GateRef value = GetVregValue(glue, sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    DISPATCH(MOV_V16_V16);
}

DECLARE_ASM_HANDLER(HandleLdaStrId16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst16_0(pc);
    varAcc = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    DISPATCH_WITH_ACC(LDA_STR_ID16);
}

DECLARE_ASM_HANDLER(HandleLdaiImm32)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef imm = ReadInst32_0(pc);
    varAcc = IntToTaggedPtr(imm);
    DISPATCH_WITH_ACC(LDAI_IMM32);
}

DECLARE_ASM_HANDLER(HandleFldaiImm64)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef imm = CastInt64ToFloat64(ReadInst64_0(pc));
    varAcc = DoubleToTaggedDoublePtr(imm);
    DISPATCH_WITH_ACC(FLDAI_IMM64);
}

DECLARE_ASM_HANDLER(HandleJeqzImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    BRANCH(TaggedIsFalse(acc), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        BRANCH(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            BRANCH(Int32Equal(TaggedGetInt(acc), Int32(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            BRANCH(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                BRANCH(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        Label dispatch(env);
        Label slowPath(env);
        GateRef offset = ReadInstSigned8_0(pc);
        UPDATE_HOTNESS(sp, callback);
        DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    DISPATCH_BAK(JUMP, INT_PTR(JEQZ_IMM8));
}

DECLARE_ASM_HANDLER(HandleJeqzImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    BRANCH(TaggedIsFalse(acc), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        BRANCH(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            BRANCH(Int32Equal(TaggedGetInt(acc), Int32(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            BRANCH(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                BRANCH(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        Label dispatch(env);
        Label slowPath(env);
        GateRef offset = ReadInstSigned16_0(pc);
        UPDATE_HOTNESS(sp, callback);
        DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    DISPATCH_BAK(JUMP, INT_PTR(JEQZ_IMM16));
}

DECLARE_ASM_HANDLER(HandleJeqzImm32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    BRANCH(TaggedIsFalse(acc), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        BRANCH(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            BRANCH(Int32Equal(TaggedGetInt(acc), Int32(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            BRANCH(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                BRANCH(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        Label dispatch(env);
        Label slowPath(env);
        GateRef offset = ReadInstSigned32_0(pc);
        UPDATE_HOTNESS(sp, callback);
        DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    DISPATCH_BAK(JUMP, INT_PTR(JEQZ_IMM32));
}

DECLARE_ASM_HANDLER(HandleJnezImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label accEqualTrue(env);
    Label accNotEqualTrue(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    BRANCH(TaggedIsTrue(acc), &accEqualTrue, &accNotEqualTrue);
    Bind(&accNotEqualTrue);
    {
        BRANCH(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            BRANCH(Int32Equal(TaggedGetInt(acc), Int32(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            BRANCH(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                BRANCH(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &last, &accEqualTrue);
            }
        }
    }
    Bind(&accEqualTrue);
    {
        Label dispatch(env);
        Label slowPath(env);
        GateRef offset = ReadInstSigned8_0(pc);
        UPDATE_HOTNESS(sp, callback);
        DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    DISPATCH_BAK(JUMP, INT_PTR(JNEZ_IMM8));
}

DECLARE_ASM_HANDLER(HandleJnezImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label accEqualTrue(env);
    Label accNotEqualTrue(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    BRANCH(TaggedIsTrue(acc), &accEqualTrue, &accNotEqualTrue);
    Bind(&accNotEqualTrue);
    {
        BRANCH(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            BRANCH(Int32Equal(TaggedGetInt(acc), Int32(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            BRANCH(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                BRANCH(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &last, &accEqualTrue);
            }
        }
    }
    Bind(&accEqualTrue);
    {
        Label dispatch(env);
        Label slowPath(env);
        GateRef offset = ReadInstSigned16_0(pc);
        UPDATE_HOTNESS(sp, callback);
        DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    DISPATCH_BAK(JUMP, INT_PTR(JNEZ_IMM16));
}

DECLARE_ASM_HANDLER(HandleJnezImm32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label accEqualTrue(env);
    Label accNotEqualTrue(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accIsDouble(env);
    Label last(env);
    BRANCH(TaggedIsTrue(acc), &accEqualTrue, &accNotEqualTrue);
    Bind(&accNotEqualTrue);
    {
        BRANCH(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            BRANCH(Int32Equal(TaggedGetInt(acc), Int32(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            BRANCH(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                BRANCH(DoubleEqual(GetDoubleOfTDouble(acc), Double(0)), &last, &accEqualTrue);
            }
        }
    }
    Bind(&accEqualTrue);
    {
        Label dispatch(env);
        Label slowPath(env);
        GateRef offset = ReadInstSigned32_0(pc);
        UPDATE_HOTNESS(sp, callback);
        DISPATCH_BAK(JUMP, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    DISPATCH_BAK(JUMP, INT_PTR(JNEZ_IMM32));
}

DECLARE_ASM_HANDLER(HandleReturn)
{
    auto env = GetEnvironment();
    METHOD_EXIT();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
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

    GateRef frame = GetFrame(*varSp);
    BRANCH(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &isStable);
    Bind(&isStable);
    {
        GateRef varProfileTypeInfoVal = *varProfileTypeInfo;
        GateRef isProfileDumpedAndJitCompiled = LogicAndBuilder(env)
            .And(ProfilerStubBuilder(env).IsCompiledOrTryCompile(glue, GetFunctionFromFrame(glue, frame),
                varProfileTypeInfoVal, callback, pc))
            .And(ProfilerStubBuilder(env).IsProfileTypeInfoDumped(varProfileTypeInfoVal, callback))
            .Done();
        BRANCH(isProfileDumpedAndJitCompiled, &tryContinue, &updateHotness);
    }
    Bind(&updateHotness);
    {
        GateRef function = GetFunctionFromFrame(glue, frame);
        GateRef method = Load(VariableType::JS_ANY(), glue, function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = LoadPrimitive(VariableType::NATIVE_POINTER(), method,
            IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = Int32Not(TruncPtrToInt32(PtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, method, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef curFunc = GetFunctionFromFrame(glue, frame);
    CallNGCRuntime(glue, RTSTUB_ID(EndCallTimer), { glue, curFunc });
#endif
    GateRef currentSp = *varSp;
    varSp = LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = LoadZeroOffsetPrimitive(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = LoadZeroOffsetPrimitive(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(*prevState);
    BRANCH(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
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
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { glue, *varAcc, *varSp, currentSp, jumpSize });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(glue, *prevState);
        GateRef method = Load(VariableType::JS_ANY(), glue, function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(glue, method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
            { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
            *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

DECLARE_ASM_HANDLER(HandleReturnundefined)
{
    auto env = GetEnvironment();
    METHOD_EXIT();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
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

    GateRef frame = GetFrame(*varSp);
    BRANCH(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &isStable);
    Bind(&isStable);
    {
        GateRef varProfileTypeInfoVal = *varProfileTypeInfo;
        GateRef isProfileDumpedAndJitCompiled = LogicAndBuilder(env)
            .And(ProfilerStubBuilder(env).IsCompiledOrTryCompile(glue, GetFunctionFromFrame(glue, frame),
                varProfileTypeInfoVal, callback, pc))
            .And(ProfilerStubBuilder(env).IsProfileTypeInfoDumped(varProfileTypeInfoVal, callback))
            .Done();
        BRANCH(isProfileDumpedAndJitCompiled, &tryContinue, &updateHotness);
    }
    Bind(&updateHotness);
    {
        GateRef function = GetFunctionFromFrame(glue, frame);
        GateRef method = Load(VariableType::JS_ANY(), glue, function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = LoadPrimitive(VariableType::NATIVE_POINTER(), method,
            IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = Int32Not(TruncPtrToInt32(PtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, method, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef curFunc = GetFunctionFromFrame(glue, frame);
    CallNGCRuntime(glue, RTSTUB_ID(EndCallTimer), { glue, curFunc });
#endif
    GateRef currentSp = *varSp;
    varSp = LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = LoadZeroOffsetPrimitive(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = LoadZeroOffsetPrimitive(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(*prevState);
    varAcc = Undefined();
    BRANCH(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
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
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { glue, *varAcc, *varSp, currentSp, jumpSize });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(glue, *prevState);
        GateRef method = Load(VariableType::JS_ANY(), glue, function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(glue, method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
            { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
            *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

DECLARE_ASM_HANDLER(HandleSuspendgeneratorV8)
{
    auto env = GetEnvironment();
    METHOD_EXIT();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
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

    GateRef genObj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_0(pc)));
    GateRef value = *varAcc;
    GateRef frame = GetFrame(*varSp);
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuspendGenerator), { genObj, value });
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, *varSp, *varPc, *varConstpool, *varProfileTypeInfo, *varAcc, *varHotnessCounter);
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
        GateRef function = GetFunctionFromFrame(glue, frame);
        GateRef method = Load(VariableType::JS_ANY(), glue, function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = LoadPrimitive(VariableType::NATIVE_POINTER(), method,
            IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = Int32Not(TruncPtrToInt32(PtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, method, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef curFunc = GetFunctionFromFrame(glue, frame);
    CallNGCRuntime(glue, RTSTUB_ID(EndCallTimer), { glue, curFunc });
#endif
    GateRef currentSp = *varSp;
    varSp = LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = LoadZeroOffsetPrimitive(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = LoadZeroOffsetPrimitive(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(*prevState);
    BRANCH(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
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
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { glue, *varAcc, *varSp, currentSp, jumpSize });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(glue, *prevState);
        GateRef method = Load(VariableType::JS_ANY(), glue, function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(glue, method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
            { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
            *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

DECLARE_ASM_HANDLER(HandleDeprecatedSuspendgeneratorPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
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

    GateRef genObj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef frame = GetFrame(*varSp);
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuspendGenerator), { genObj, value });
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, *varSp, *varPc, *varConstpool, *varProfileTypeInfo, *varAcc, *varHotnessCounter);
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
        GateRef function = GetFunctionFromFrame(glue, frame);
        GateRef method = Load(VariableType::JS_ANY(), glue, function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = LoadPrimitive(VariableType::NATIVE_POINTER(), method,
            IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = Int32Not(TruncPtrToInt32(PtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, method, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef curFunc = GetFunctionFromFrame(glue, frame);
    CallNGCRuntime(glue, RTSTUB_ID(EndCallTimer), { glue, curFunc });
#endif
    GateRef currentSp = *varSp;
    varSp = LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = LoadZeroOffsetPrimitive(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = LoadZeroOffsetPrimitive(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(*prevState);
    BRANCH(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
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
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { glue, *varAcc, *varSp, currentSp, jumpSize });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(glue, *prevState);
        GateRef method = Load(VariableType::JS_ANY(), glue, function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(glue, method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
            { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
            *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

DECLARE_ASM_HANDLER(HandleTryldglobalbynameImm8Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_1, StringIdInfo::Length::BITS_16);
    GateRef result = builder.TryLoadGlobalByName(glue, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(TRYLDGLOBALBYNAME_IMM8_ID16));
}

DECLARE_ASM_HANDLER(HandleTryldglobalbynameImm16Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_2, StringIdInfo::Length::BITS_16);
    GateRef result = builder.TryLoadGlobalByName(glue, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(TRYLDGLOBALBYNAME_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleTrystglobalbynameImm8Id16)
{
    GateRef slotId = ZExtInt16ToInt32(ReadInst8_0(pc));
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_1, StringIdInfo::Length::BITS_16);
    GateRef result = builder.TryStoreGlobalByName(glue, 0, info, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(TRYSTGLOBALBYNAME_IMM8_ID16));
}

DECLARE_ASM_HANDLER(HandleTrystglobalbynameImm16Id16)
{
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_2, StringIdInfo::Length::BITS_16);
    GateRef result = builder.TryStoreGlobalByName(glue, 0, info, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(TRYSTGLOBALBYNAME_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleLdglobalvarImm16Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_2, StringIdInfo::Length::BITS_16);
    GateRef result = builder.LoadGlobalVar(glue, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDGLOBALVAR_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleStglobalvarImm16Id16)
{
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_2, StringIdInfo::Length::BITS_16);
    GateRef result = builder.StoreGlobalVar(glue, 0, info, acc, profileTypeInfo, slotId);
    CHECK_EXCEPTION(result, INT_PTR(STGLOBALVAR_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleCreateregexpwithliteralImm8Id16Imm8)
{
    GateRef stringId = ReadInst16_1(pc);
    GateRef pattern = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef flags = ReadInst8_3(pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateRegExpWithLiteral),
                              { pattern, Int8ToTaggedInt(flags) });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8));
}

DECLARE_ASM_HANDLER(HandleCreateregexpwithliteralImm16Id16Imm8)
{
    GateRef stringId = ReadInst16_2(pc);
    GateRef pattern = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef flags = ReadInst8_4(pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateRegExpWithLiteral),
                              { pattern, Int8ToTaggedInt(flags) });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8));
}

DECLARE_ASM_HANDLER(HandleIstrue)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastToBoolean(glue, *varAcc, true);
    DISPATCH_WITH_ACC(ISTRUE);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeIstruePrefImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastToBooleanWithProfile(glue, *varAcc, callback, true);
    DISPATCH_WITH_ACC(CALLRUNTIME_ISTRUE_PREF_IMM8);
}

DECLARE_ASM_HANDLER(HandleIsfalse)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastToBoolean(glue, *varAcc, false);
    DISPATCH_WITH_ACC(ISFALSE);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeIsfalsePrefImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastToBooleanWithProfile(glue, *varAcc, callback, false);
    DISPATCH_WITH_ACC(CALLRUNTIME_ISFALSE_PREF_IMM8);
}

DECLARE_ASM_HANDLER(HandleTonumberImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef value = *varAcc;
    Label valueIsNumber(env);
    Label valueNotNumber(env);
    BRANCH(TaggedIsNumber(value), &valueIsNumber, &valueNotNumber);
    Bind(&valueIsNumber);
    {
        varAcc = value;
        DISPATCH_WITH_ACC(TONUMBER_IMM8);
    }
    Bind(&valueNotNumber);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumber), { value });
        CHECK_EXCEPTION_WITH_VARACC(res, INT_PTR(TONUMBER_IMM8));
    }
}

DECLARE_ASM_HANDLER(HandleDeprecatedTonumberPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    Label valueIsNumber(env);
    Label valueNotNumber(env);
    BRANCH(TaggedIsNumber(value), &valueIsNumber, &valueNotNumber);
    Bind(&valueIsNumber);
    {
        varAcc = value;
        DISPATCH_WITH_ACC(DEPRECATED_TONUMBER_PREF_V8);
    }
    Bind(&valueNotNumber);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumber), { value });
        CHECK_EXCEPTION_WITH_VARACC(res, INT_PTR(DEPRECATED_TONUMBER_PREF_V8));
    }
}

DECLARE_ASM_HANDLER(HandleAdd2Imm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Add(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(ADD2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleSub2Imm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    OperationsStubBuilder builder(this);
    GateRef result = builder.Sub(glue, left, acc, callback);
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(SUB2_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleLdbigintId16)
{
    GateRef stringId = ReadInst16_0(pc);
    GateRef numberBigInt = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef res = CallRuntime(glue, RTSTUB_ID(LdBigInt), { numberBigInt });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(LDBIGINT_ID16));
}

DECLARE_ASM_HANDLER(HandleTonumericImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef value = *varAcc;
    Label valueIsNumeric(env);
    Label valueNotNumeric(env);
    BRANCH(TaggedIsNumeric(glue, value), &valueIsNumeric, &valueNotNumeric);
    Bind(&valueIsNumeric);
    {
        if (!callback.IsEmpty()) {
            Label valueIsNumber(env);
            Label profilerEnd(env);
            BRANCH(TaggedIsNumber(value), &valueIsNumber, &profilerEnd);
            Bind(&valueIsNumber);
            {
                Label valueIsInt(env);
                Label valueIsDouble(env);
                BRANCH(TaggedIsInt(value), &valueIsInt, &valueIsDouble);
                Bind(&valueIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                    Jump(&profilerEnd);
                }
                Bind(&valueIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
                    Jump(&profilerEnd);
                }
            }
            Bind(&profilerEnd);
        }
        varAcc = value;
        DISPATCH_WITH_ACC(TONUMERIC_IMM8);
    }
    Bind(&valueNotNumeric);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumeric), { value });
        CHECK_EXCEPTION_WITH_VARACC(res, INT_PTR(TONUMERIC_IMM8));
    }
}

DECLARE_ASM_HANDLER(HandleDeprecatedTonumericPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    Label valueIsNumeric(env);
    Label valueNotNumeric(env);
    BRANCH(TaggedIsNumeric(glue, value), &valueIsNumeric, &valueNotNumeric);
    Bind(&valueIsNumeric);
    {
        varAcc = value;
        DISPATCH_WITH_ACC(DEPRECATED_TONUMERIC_PREF_V8);
    }
    Bind(&valueNotNumeric);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumeric), { value });
        CHECK_EXCEPTION_WITH_VARACC(res, INT_PTR(DEPRECATED_TONUMERIC_PREF_V8));
    }
}

DECLARE_ASM_HANDLER(HandleDynamicimport)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef specifier = *varAcc;
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef res = CallRuntime(glue, RTSTUB_ID(DynamicImport), { specifier, currentFunc });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(DYNAMICIMPORT));
}

DECLARE_ASM_HANDLER(HandleDeprecatedDynamicimportPrefV8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef specifier = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef res = CallRuntime(glue, RTSTUB_ID(DynamicImport), { specifier, currentFunc });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(DEPRECATED_DYNAMICIMPORT_PREF_V8));
}

DECLARE_ASM_HANDLER(HandleCreateasyncgeneratorobjV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_0(pc);
    GateRef genFunc = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateAsyncGeneratorObj), { genFunc });
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(CREATEASYNCGENERATOROBJ_V8);
}

DECLARE_ASM_HANDLER(HandleAsyncgeneratorresolveV8V8V8)
{
    auto env = GetEnvironment();
    METHOD_EXIT();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
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

    GateRef asyncGenerator = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_0(pc)));
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef flag = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef frame = GetFrame(*varSp);
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncGeneratorResolve),
                              { asyncGenerator, value, flag });
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
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
        GateRef function = GetFunctionFromFrame(glue, frame);
        GateRef method = Load(VariableType::JS_ANY(), glue, function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = LoadPrimitive(VariableType::NATIVE_POINTER(), method,
            IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = Int32Not(TruncPtrToInt32(PtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp, callback);
        SetHotnessCounter(glue, method, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    GateRef curFunc = GetFunctionFromFrame(glue, frame);
    CallNGCRuntime(glue, RTSTUB_ID(EndCallTimer), { glue, curFunc });
#endif
    GateRef currentSp = *varSp;
    varSp = LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));

    GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef maybeFrameType = LoadZeroOffsetPrimitive(VariableType::INT64(), typePos);
    BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
           &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
    Bind(&isBaselineBuiltinFrame);
    {
        varSp = LoadZeroOffsetPrimitive(VariableType::NATIVE_POINTER(), *varSp);
        Jump(&notBaselineBuiltinFrame);
    }
    Bind(&notBaselineBuiltinFrame);
    prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(*prevState);
    BRANCH(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
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
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturnBaseline), { glue, *varAcc, *varSp, currentSp, jumpSize });
        Return();
    }
    Bind(&pcNotEqualBaseline);
    {
        GateRef function = GetFunctionFromFrame(glue, *prevState);
        GateRef method = Load(VariableType::JS_ANY(), glue, function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(glue, method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(*prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
            { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
            *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

DECLARE_ASM_HANDLER(HandleAsyncgeneratorrejectV8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef asyncGenerator = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_0(pc)));
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncGeneratorReject),
                              { asyncGenerator, acc });
    CHECK_EXCEPTION_VARACC(res, INT_PTR(ASYNCGENERATORREJECT_V8));
}

DECLARE_ASM_HANDLER(HandleSetgeneratorstateImm8)
{
    GateRef index = ReadInst8_0(pc);
    GateRef value = acc;
    CallRuntime(glue, RTSTUB_ID(SetGeneratorState), { value, IntToTaggedInt(index) });
    DISPATCH(SETGENERATORSTATE_IMM8);
}

DECLARE_ASM_HANDLER(HandleDeprecatedAsyncgeneratorrejectPrefV8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef asyncGenerator = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncGeneratorReject),
                              { asyncGenerator, value });
    CHECK_EXCEPTION_VARACC(res, INT_PTR(DEPRECATED_ASYNCGENERATORREJECT_PREF_V8_V8));
}

DECLARE_ASM_HANDLER(HandleSupercallthisrangeImm8Imm8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef range = ZExtInt8ToInt16(ReadInst8_1(pc));
    GateRef v0 = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef actualNumArgs = ZExtInt16ToInt32(range);
    GateRef thisFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef newTarget = GetNewTarget(glue, sp);
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

    BRANCH(TaggedIsHeapObject(superCtor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    BRANCH(IsJSFunction(glue, superCtor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    BRANCH(IsConstructor(glue, superCtor), &ctorIsConstructor, &slowPath);
    Bind(&ctorIsConstructor);
    BRANCH(TaggedIsUndefined(newTarget), &slowPath, &fastPath);
    Bind(&fastPath);
    {
        BRANCH(IsBase(glue, superCtor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            NewObjectStubBuilder newBuilder(this);
            thisObj = newBuilder.FastSuperAllocateThis(glue, superCtor, newTarget);
            BRANCH(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(ZExtInt16ToPtr(v0), IntPtr(JSTaggedValue::TaggedTypeSize()))); // skip function
        GateRef jumpSize = IntPtr(InterpreterAssembly::GetCallSize(EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8));
        METHOD_ENTRY_ENV_DEFINED(superCtor);
        JSCallArgs callArgs(JSCallMode::SUPER_CALL_WITH_ARGV);
        callArgs.superCallArgs = {
            thisFunc, Int16ToTaggedInt(v0), ZExtInt32ToPtr(actualNumArgs), argv, *thisObj, newTarget
        };
        CallStubBuilder callBuilder(this, glue, superCtor, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs,
            callback);
        res = callBuilder.JSCallDispatch();
        Jump(&threadCheck);
    }
    Bind(&slowPath);
    res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { thisFunc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
    Jump(&checkResult);
    Bind(&checkResult);
    {
        BRANCH(TaggedIsException(*res), &isException, &dispatch);
    }
    Bind(&threadCheck);
    {
        BRANCH(HasPendingException(glue), &isException, &dispatch);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&dispatch);
    varAcc = *res;
    DISPATCH_WITH_ACC(SUPERCALLTHISRANGE_IMM8_IMM8_V8);
}

DECLARE_ASM_HANDLER(HandleSupercallarrowrangeImm8Imm8V8)
{
    GateRef range = ReadInst8_1(pc);
    GateRef v0 = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { acc, Int16ToTaggedInt(v0), Int8ToTaggedInt(range) });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(SUPERCALLARROWRANGE_IMM8_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleWideSupercallthisrangePrefImm16V8)
{
    GateRef range = ReadInst16_1(pc);
    GateRef v0 = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { currentFunc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8));
}

DECLARE_ASM_HANDLER(HandleWideSupercallarrowrangePrefImm16V8)
{
    GateRef range = ReadInst16_1(pc);
    GateRef v0 = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { acc, Int16ToTaggedInt(v0), Int16ToTaggedInt(range) });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8));
}

DECLARE_ASM_HANDLER(HandleLdsuperbyvalueImm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, propKey }); // sp for thisFunc
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(LDSUPERBYVALUE_IMM8_V8));
}
DECLARE_ASM_HANDLER(HandleLdsuperbyvalueImm16V8)
{
    GateRef v0 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = acc;
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, propKey }); // sp for thisFunc
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(LDSUPERBYVALUE_IMM16_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedLdsuperbyvaluePrefV8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, propKey }); // sp for thisFunc
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedGetiteratornextPrefV8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef method = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetIteratorNext), { obj, method });
    CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_GETITERATORNEXT_PREF_V8_V8));
}

DECLARE_ASM_HANDLER(HandleLdobjbyvalueImm8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = acc;
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.LoadObjByValue(glue, receiver, propKey, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDOBJBYVALUE_IMM8_V8));
}

DECLARE_ASM_HANDLER(HandleLdobjbyvalueImm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = acc;
    GateRef slotId = ZExtInt8ToInt32(ReadInst16_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.LoadObjByValue(glue, receiver, propKey, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDOBJBYVALUE_IMM16_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedLdobjbyvaluePrefV8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.DeprecatedLoadObjByValue(glue, receiver, propKey);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(DEPRECATED_LDOBJBYVALUE_PREF_V8_V8));
}

DECLARE_ASM_HANDLER(HandleLdsuperbynameImm8Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst16_1(pc);
    GateRef receiver = acc;
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { receiver, propKey });
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDSUPERBYNAME_IMM8_ID16));
}

DECLARE_ASM_HANDLER(HandleLdsuperbynameImm16Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst16_2(pc);
    GateRef receiver = acc;
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { receiver, propKey });
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDSUPERBYNAME_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleDeprecatedLdsuperbynamePrefId32V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef v0 = ReadInst8_5(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { receiver, propKey });
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8));
}

DECLARE_ASM_HANDLER(HandleLdobjbyindexImm8Imm16)
{
    auto env = GetEnvironment();

    GateRef receiver = acc;
    GateRef index = ZExtInt16ToInt32(ReadInst16_1(pc));
    Label fastPath(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, receiver, index, callback);
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(LDOBJBYINDEX_IMM8_IMM16));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(LDOBJBYINDEX_IMM8_IMM16));
    }
}

DECLARE_ASM_HANDLER(HandleLdobjbyindexImm16Imm16)
{
    auto env = GetEnvironment();

    GateRef receiver = acc;
    GateRef index = ZExtInt16ToInt32(ReadInst16_2(pc));
    Label fastPath(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, receiver, index, callback);
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(LDOBJBYINDEX_IMM16_IMM16));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(LDOBJBYINDEX_IMM16_IMM16));
    }
}

DECLARE_ASM_HANDLER(HandleWideLdobjbyindexPrefImm32)
{
    auto env = GetEnvironment();

    GateRef receiver = acc;
    GateRef index = ReadInst32_1(pc);
    Label fastPath(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, receiver, index, callback);
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(WIDE_LDOBJBYINDEX_PREF_IMM32));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(WIDE_LDOBJBYINDEX_PREF_IMM32));
    }
}

DECLARE_ASM_HANDLER(HandleDeprecatedLdobjbyindexPrefV8Imm32)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef index = ReadInst32_2(pc);
    Label fastPath(env);
    Label slowPath(env);
    BRANCH(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = GetPropertyByIndex(glue, receiver, index, callback);
        Label notHole(env);
        BRANCH(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32));
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        CHECK_EXCEPTION_WITH_ACC(result, INT_PTR(DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32));
    }
}

DECLARE_ASM_HANDLER(HandleStconsttoglobalrecordImm16Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst16_2(pc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedTrue() });
    CHECK_EXCEPTION_VARACC(result, INT_PTR(STCONSTTOGLOBALRECORD_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleSttoglobalrecordImm16Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst16_2(pc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, ZExtInt16ToInt32(stringId));
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedFalse() });
    CHECK_EXCEPTION_VARACC(result, INT_PTR(STTOGLOBALRECORD_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleDeprecatedStconsttoglobalrecordPrefId32)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedTrue() });
    CHECK_EXCEPTION_VARACC(result, INT_PTR(DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32));
}

DECLARE_ASM_HANDLER(HandleDeprecatedStlettoglobalrecordPrefId32)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedFalse() });
    CHECK_EXCEPTION_VARACC(result, INT_PTR(DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32));
}

DECLARE_ASM_HANDLER(HandleDeprecatedStclasstoglobalrecordPrefId32)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedFalse() });
    CHECK_EXCEPTION_VARACC(result, INT_PTR(DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32));
}

DECLARE_ASM_HANDLER(HandleGetmodulenamespaceImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef index = ReadInst8_0(pc);
#if ENABLE_NEXT_OPTIMIZATION
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef moduleRef = LoadModuleNamespaceByIndex(glue, ZExtInt8ToInt32(index), module);
#else
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespaceByIndex), { IntToTaggedInt(index) });
#endif
    varAcc = moduleRef;
    DISPATCH_WITH_ACC(GETMODULENAMESPACE_IMM8);
}

DECLARE_ASM_HANDLER(HandleWideGetmodulenamespacePrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef index = ReadInst16_1(pc);
#if ENABLE_NEXT_OPTIMIZATION
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef moduleRef = LoadModuleNamespaceByIndex(glue, ZExtInt16ToInt32(index), module);
#else
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespaceByIndex), { Int16ToTaggedInt(index) });
#endif
    varAcc = moduleRef;
    DISPATCH_WITH_ACC(WIDE_GETMODULENAMESPACE_PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleDeprecatedGetmodulenamespacePrefId32)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetStringFromConstPool(glue, constpool, stringId);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespace), { prop });
    varAcc = moduleRef;
    DISPATCH_WITH_ACC(DEPRECATED_GETMODULENAMESPACE_PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleLdlocalmodulevarImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef index = ReadInst8_0(pc);
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    varAcc = Loadlocalmodulevar(glue, index, module);
    DISPATCH_WITH_ACC(LDLOCALMODULEVAR_IMM8);
}

DECLARE_ASM_HANDLER(HandleWideLdlocalmodulevarPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef index = ReadInst16_1(pc);
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    varAcc = Loadlocalmodulevar(glue, index, module);
    DISPATCH_WITH_ACC(WIDE_LDLOCALMODULEVAR_PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleLdexternalmodulevarImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef index = ReadInst8_0(pc);
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
#if ENABLE_NEXT_OPTIMIZATION
    GateRef indexInt32 = ZExtInt8ToInt32(index);
    varAcc = LoadExternalmodulevar(glue, indexInt32, module);
#else
    varAcc = CallRuntime(glue, RTSTUB_ID(LdExternalModuleVarByIndexWithModule), {Int8ToTaggedInt(index), module});
#endif
    DISPATCH_WITH_ACC(LDEXTERNALMODULEVAR_IMM8);
}

DECLARE_ASM_HANDLER(HandleWideLdexternalmodulevarPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef index = ReadInst16_1(pc);
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
#if ENABLE_NEXT_OPTIMIZATION
    GateRef indexInt32 = ZExtInt16ToInt32(index);
    varAcc = LoadExternalmodulevar(glue, indexInt32, module);
#else
    varAcc = CallRuntime(glue, RTSTUB_ID(LdExternalModuleVarByIndexWithModule), {Int16ToTaggedInt(index), module});
#endif
    DISPATCH_WITH_ACC(WIDE_LDEXTERNALMODULEVAR_PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleDeprecatedLdmodulevarPrefId32Imm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef flag = ZExtInt8ToInt32(ReadInst8_5(pc));
    GateRef key = GetStringFromConstPool(glue, constpool, stringId);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(LdModuleVar), { key, IntToTaggedInt(flag) });
    varAcc = moduleRef;
    DISPATCH_WITH_ACC(DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8);
}

DECLARE_ASM_HANDLER(HandleStmodulevarImm8)
{
    GateRef index = ReadInst8_0(pc);
    GateRef value = acc;

    CallRuntime(glue, RTSTUB_ID(StModuleVarByIndex), { IntToTaggedInt(index), value });
    DISPATCH(STMODULEVAR_IMM8);
}

DECLARE_ASM_HANDLER(HandleWideStmodulevarPrefImm16)
{
    GateRef index = ReadInst16_1(pc);
    GateRef value = acc;

    CallRuntime(glue, RTSTUB_ID(StModuleVarByIndex), { Int16ToTaggedInt(index), value });
    DISPATCH(WIDE_STMODULEVAR_PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleDeprecatedStmodulevarPrefId32)
{
    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetStringFromConstPool(glue, constpool, stringId);
    GateRef value = acc;

    CallRuntime(glue, RTSTUB_ID(StModuleVar), { prop, value });
    DISPATCH(DEPRECATED_STMODULEVAR_PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleNewlexenvImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    auto env = GetEnvironment();
    GateRef numVars = ReadInst8_0(pc);
    GateRef state = GetFrame(sp);
    auto parent = GetEnvFromFrame(glue, state);
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
    DISPATCH_WITH_ACC(NEWLEXENV_IMM8);
}

DECLARE_ASM_HANDLER(HandleWideNewlexenvPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    auto env = GetEnvironment();
    GateRef numVars = ReadInst16_1(pc);
    GateRef state = GetFrame(sp);
    auto parent = GetEnvFromFrame(glue, state);
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
    DISPATCH_WITH_ACC(WIDE_NEWLEXENV_PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleNewlexenvwithnameImm8Id16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef numVars = ZExtInt8ToInt16(ReadInst8_0(pc));
    GateRef scopeId = ReadInst16_1(pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewLexicalEnvWithName),
                              { Int16ToTaggedInt(numVars), Int16ToTaggedInt(scopeId) });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
    Bind(&notException);
    varAcc = res;
    GateRef state = GetFrame(sp);
    SetEnvToFrame(glue, state, res);
    DISPATCH_WITH_ACC(NEWLEXENVWITHNAME_IMM8_ID16);
}

DECLARE_ASM_HANDLER(HandleWideNewlexenvwithnamePrefImm16Id16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef numVars = ReadInst16_1(pc);
    GateRef scopeId = ReadInst16_3(pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewLexicalEnvWithName),
                              { Int16ToTaggedInt(numVars), Int16ToTaggedInt(scopeId) });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
    Bind(&notException);
    varAcc = res;
    GateRef state = GetFrame(sp);
    SetEnvToFrame(glue, state, res);
    DISPATCH_WITH_ACC(WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16);
}

DECLARE_ASM_HANDLER(HandleDefineclasswithbufferImm8Id16Id16Imm16V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef methodId = ReadInst16_1(pc);
    GateRef literalId = ReadInst16_3(pc);
    GateRef length = ReadInst16_5(pc);
    GateRef v0 = ReadInst8_7(pc);

    GateRef proto = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef lexicalEnv = GetEnvFromFrame(glue, GetFrame(sp));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer),
                              { proto, lexicalEnv, constpool,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId), module,
                                Int16ToTaggedInt(length)});

    Label isException(env);
    Label isNotException(env);
    BRANCH(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&isNotException);
#if ECMASCRIPT_ENABLE_IC
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    UpdateProfileTypeInfoCellToFunction(glue, res, profileTypeInfo, slotId);
    callback.ProfileDefineClass(res);
#endif
    varAcc = res;
    DISPATCH_WITH_ACC(DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8);
}

DECLARE_ASM_HANDLER(HandleDefineclasswithbufferImm16Id16Id16Imm16V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef methodId = ReadInst16_2(pc);
    GateRef literalId = ReadInst16_4(pc);
    GateRef length = ReadInst16_6(pc);
    GateRef v0 = ReadInst8_8(pc);

    GateRef proto = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef lexicalEnv = GetEnvFromFrame(glue, GetFrame(sp));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer),
                              { proto, lexicalEnv, constpool,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId), module,
                                Int16ToTaggedInt(length)});

    Label isException(env);
    Label isNotException(env);
    BRANCH(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&isNotException);
#if ECMASCRIPT_ENABLE_IC
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    UpdateProfileTypeInfoCellToFunction(glue, res, profileTypeInfo, slotId);
    callback.ProfileDefineClass(res);
#endif
    varAcc = res;
    DISPATCH_WITH_ACC(DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8);
}

DECLARE_ASM_HANDLER(HandleDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef methodId = ReadInst16_1(pc);
    GateRef literalId = ReadInst16_3(pc);
    GateRef length = ReadInst16_5(pc);
    GateRef v0 = ReadInst8_7(pc);
    GateRef v1 = ReadInst8_8(pc);

    GateRef lexicalEnv = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef proto = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));

    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer),
                              { proto, lexicalEnv, constpool,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId), module,
                                Int16ToTaggedInt(length)});

    Label isException(env);
    Label isNotException(env);
    BRANCH(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&isNotException);
    varAcc = res;
    DISPATCH_WITH_ACC(DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8);
}

DECLARE_ASM_HANDLER(HandleLdobjbynameImm8Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef receiver = acc;
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_1, StringIdInfo::Length::BITS_16);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDOBJBYNAME_IMM8_ID16));
}

DECLARE_ASM_HANDLER(HandleLdobjbynameImm16Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    GateRef receiver = acc;
    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_2, StringIdInfo::Length::BITS_16);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDOBJBYNAME_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleDeprecatedLdobjbynamePrefId32V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef receiver = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetStringFromConstPool(glue, constpool, stringId);
    AccessObjectStubBuilder builder(this);
    GateRef result = builder.DeprecatedLoadObjByName(glue, receiver, propKey);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(DEPRECATED_LDOBJBYNAME_PREF_ID32_V8));
}

DECLARE_ASM_HANDLER(HandleCallarg0Imm8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG0);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLARG0_IMM8);
    JSCallArgs callArgs(JSCallMode::CALL_ARG0);
    callArgs.callArgs = { 0, 0, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleDeprecatedCallarg0PrefV8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG0);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef func = GetVregValue(glue, sp, ZExtInt8ToPtr(funcReg));
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARG0_PREF_V8);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_ARG0);
    callArgs.callArgs = { 0, 0, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallarg1Imm8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG1);
    GateRef a0 = ReadInst8_1(pc);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef jumpSize = INT_PTR(CALLARG1_IMM8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_ARG1);
    callArgs.callArgs = { a0Value, 0, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleDeprecatedCallarg1PrefV8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG1);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
    GateRef func = GetVregValue(glue, sp, ZExtInt8ToPtr(funcReg));
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARG1_PREF_V8_V8);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_ARG1);
    callArgs.callArgs = { a0Value, 0, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallargs2Imm8V8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS2);
    GateRef a0 = ReadInst8_1(pc);
    GateRef a1 = ReadInst8_2(pc);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a1));
    GateRef jumpSize = INT_PTR(CALLARGS2_IMM8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_ARG2);
    callArgs.callArgs = { a0Value, a1Value, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleDeprecatedCallargs2PrefV8V8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS2);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
    GateRef a1 = ReadInst8_3(pc);
    GateRef func = GetVregValue(glue, sp, ZExtInt8ToPtr(funcReg));
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a1));
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARGS2_PREF_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_ARG2);
    callArgs.callArgs = { a0Value, a1Value, 0 };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallargs3Imm8V8V8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS3);
    GateRef a0 = ReadInst8_1(pc);
    GateRef a1 = ReadInst8_2(pc);
    GateRef a2 = ReadInst8_3(pc);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a1));
    GateRef a2Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a2));
    GateRef jumpSize = INT_PTR(CALLARGS3_IMM8_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_ARG3);
    callArgs.callArgs = { a0Value, a1Value, a2Value };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleDeprecatedCallargs3PrefV8V8V8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS3);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
    GateRef a1 = ReadInst8_3(pc);
    GateRef a2 = ReadInst8_4(pc);
    GateRef func = GetVregValue(glue, sp, ZExtInt8ToPtr(funcReg));
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a1));
    GateRef a2Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a2));
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_ARG3);
    callArgs.callArgs = { a0Value, a1Value, a2Value };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallrangeImm8Imm8V8)
{
    GateRef actualNumArgs = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef argv = PtrAdd(sp, PtrMul(ZExtInt8ToPtr(ReadInst8_2(pc)), IntPtr(8))); // 8: byteSize
    GateRef jumpSize = INT_PTR(CALLRANGE_IMM8_IMM8_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::CALL_WITH_ARGV);
    callArgs.callArgv = { numArgs, argv };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleWideCallrangePrefImm16V8)
{
    GateRef actualNumArgs = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef argv = PtrAdd(sp, PtrMul(ZExtInt8ToPtr(ReadInst8_2(pc)), IntPtr(8))); // 8: byteSize
    GateRef jumpSize = INT_PTR(WIDE_CALLRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::CALL_WITH_ARGV);
    callArgs.callArgv = { numArgs, argv };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleDeprecatedCallrangePrefImm16V8)
{
    GateRef actualNumArgs = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef funcReg = ReadInst8_3(pc);
    GateRef func = GetVregValue(glue, sp, ZExtInt8ToPtr(funcReg));
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(ZExtInt8ToPtr(funcReg), IntPtr(1)), IntPtr(8))); // 1: skip function
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_WITH_ARGV);
    callArgs.callArgv = { numArgs, argv };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallthisrangeImm8Imm8V8)
{
    GateRef actualNumArgs = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef thisReg = ZExtInt8ToPtr(ReadInst8_2(pc));
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef thisValue = GetVregValue(glue, sp, thisReg);
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(thisReg, IntPtr(1)), IntPtr(8))); // 1: skip this
    GateRef jumpSize = INT_PTR(CALLTHISRANGE_IMM8_IMM8_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_WITH_ARGV);
    callArgs.callArgvWithThis = { numArgs, argv, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleWideCallthisrangePrefImm16V8)
{
    GateRef actualNumArgs = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef thisReg = ZExtInt8ToPtr(ReadInst8_3(pc));
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef thisValue = GetVregValue(glue, sp, thisReg);
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(thisReg, IntPtr(1)), IntPtr(8))); // 1: skip this
    GateRef jumpSize = INT_PTR(WIDE_CALLTHISRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_WITH_ARGV);
    callArgs.callArgvWithThis = { numArgs, argv, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleDeprecatedCallthisrangePrefImm16V8)
{
    GateRef actualNumArgs = Int32Sub(ZExtInt16ToInt32(ReadInst16_1(pc)), Int32(1));  // 1: exclude this
    GateRef funcReg = ReadInst8_3(pc);
    funcReg = ZExtInt8ToPtr(funcReg);
    GateRef func = GetVregValue(glue, sp, funcReg);
    GateRef thisValue = GetVregValue(glue, sp, PtrAdd(funcReg, IntPtr(1)));
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(funcReg, IntPtr(2)), IntPtr(8))); // 2: skip function&this
    GateRef jumpSize = INT_PTR(DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV);
    callArgs.callArgvWithThis = { numArgs, argv, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallthis0Imm8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG0);
    GateRef thisValue = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLTHIS0_IMM8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG0);
    callArgs.callArgsWithThis = { 0, 0, 0, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallthis1Imm8V8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG1);
    GateRef thisValue = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef a0 = ReadInst8_2(pc);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef jumpSize = INT_PTR(CALLTHIS1_IMM8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG1);
    callArgs.callArgsWithThis = { a0Value, 0, 0, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallthis2Imm8V8V8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS2);
    GateRef thisValue = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef a0 = ReadInst8_2(pc);
    GateRef a1 = ReadInst8_3(pc);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a1));
    GateRef jumpSize = INT_PTR(CALLTHIS2_IMM8_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG2);
    callArgs.callArgsWithThis = { a0Value, a1Value, 0, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallthis3Imm8V8V8V8V8)
{
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS3);
    GateRef thisValue = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef a0 = ReadInst8_2(pc);
    GateRef a1 = ReadInst8_3(pc);
    GateRef a2 = ReadInst8_4(pc);
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef a0Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a1));
    GateRef a2Value = GetVregValue(glue, sp, ZExtInt8ToPtr(a2));
    GateRef jumpSize = INT_PTR(CALLTHIS3_IMM8_V8_V8_V8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3);
    callArgs.callArgsWithThis = { a0Value, a1Value, a2Value, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCreatearraywithbufferImm8Id16)
{
    GateRef imm = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, currentFunc, { pc, 0, true }, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CREATEARRAYWITHBUFFER_IMM8_ID16));
}

DECLARE_ASM_HANDLER(HandleCreatearraywithbufferImm16Id16)
{
    GateRef imm = ZExtInt16ToInt32(ReadInst16_2(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));

    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, currentFunc, { pc, 0, true }, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CREATEARRAYWITHBUFFER_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleDeprecatedCreatearraywithbufferPrefImm16)
{
    GateRef imm = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateArrayWithBuffer(
        glue, imm, currentFunc, { pc, 0, true }, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16));
}

DECLARE_ASM_HANDLER(HandleCreateobjectwithbufferImm8Id16)
{
    GateRef imm = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    GateRef currentEnv = GetEnvFromFrame(glue, GetFrame(sp));
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateObjectHavingMethod(glue, result, currentEnv);

    auto env = GetEnvironment();
    Label isException(env);
    Label isNotException(env);
    BRANCH(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    DISPATCH_LAST();
    Bind(&isNotException);
    callback.ProfileCreateObject(res);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), res);
    DISPATCH_VARACC(INT_PTR(CREATEOBJECTWITHBUFFER_IMM8_ID16));
}

DECLARE_ASM_HANDLER(HandleCreateobjectwithbufferImm16Id16)
{
    GateRef imm = ZExtInt16ToInt32(ReadInst16_2(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    GateRef currentEnv = GetEnvFromFrame(glue, GetFrame(sp));
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateObjectHavingMethod(glue, result, currentEnv);

    auto env = GetEnvironment();
    Label isException(env);
    Label isNotException(env);
    BRANCH(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    DISPATCH_LAST();
    Bind(&isNotException);
    callback.ProfileCreateObject(res);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), res);
    DISPATCH_VARACC(INT_PTR(CREATEOBJECTWITHBUFFER_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleDeprecatedCreateobjectwithbufferPrefImm16)
{
    GateRef imm = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithBuffer), { result });
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16));
}

DECLARE_ASM_HANDLER(HandleNewobjrangeImm8Imm8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef numArgs = ZExtInt8ToInt16(ReadInst8_1(pc));
    GateRef firstArgRegIdx = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef firstArgOffset = Int16(1);
    GateRef ctor = GetVregValue(glue, sp, ZExtInt16ToPtr(firstArgRegIdx));
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

    BRANCH(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    BRANCH(IsJSFunction(glue, ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    BRANCH(IsConstructor(glue, ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        BRANCH(IsBase(glue, ctor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            NewObjectStubBuilder newBuilder(this);
            thisObj = newBuilder.FastNewThisObject(glue, ctor);
            BRANCH(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(
            PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
        GateRef jumpSize = IntPtr(InterpreterAssembly::GetCallSize(EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8));
        METHOD_ENTRY_ENV_DEFINED(ctor);
        JSCallArgs callArgs(JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV);
        callArgs.callConstructorArgs = { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj };
        CallStubBuilder callBuilder(this, glue, ctor, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs,
            callback);
        res = callBuilder.JSCallDispatch();
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
        BRANCH(TaggedIsException(*res), &isException, &dispatch);
    }
    Bind(&threadCheck);
    {
        BRANCH(HasPendingException(glue), &isException, &dispatch);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&dispatch);
    varAcc = *res;
    DISPATCH_WITH_ACC(NEWOBJRANGE_IMM8_IMM8_V8);
}

DECLARE_ASM_HANDLER(HandleNewobjrangeImm16Imm8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef numArgs = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef firstArgRegIdx = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef firstArgOffset = Int16(1);
    GateRef ctor = GetVregValue(glue, sp, ZExtInt16ToPtr(firstArgRegIdx));
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

    BRANCH(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    BRANCH(IsJSFunction(glue, ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    BRANCH(IsConstructor(glue, ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        BRANCH(IsBase(glue, ctor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            NewObjectStubBuilder newBuilder(this);
            thisObj = newBuilder.FastNewThisObject(glue, ctor);
            BRANCH(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(
            PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
        GateRef jumpSize =
            IntPtr(-static_cast<int64_t>(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16_IMM8_V8)));
        METHOD_ENTRY_ENV_DEFINED(ctor);
        JSCallArgs callArgs(JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV);
        callArgs.callConstructorArgs = { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj };
        CallStubBuilder callBuilder(this, glue, ctor, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs,
            callback);
        res = callBuilder.JSCallDispatch();
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
        BRANCH(TaggedIsException(*res), &isException, &dispatch);
    }
    Bind(&threadCheck);
    {
        BRANCH(HasPendingException(glue), &isException, &dispatch);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&dispatch);
    varAcc = *res;
    DISPATCH_WITH_ACC(NEWOBJRANGE_IMM16_IMM8_V8);
}

DECLARE_ASM_HANDLER(HandleWideNewobjrangePrefImm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef numArgs = ReadInst16_1(pc);
    GateRef firstArgRegIdx = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef firstArgOffset = Int16(1);
    GateRef ctor = GetVregValue(glue, sp, ZExtInt16ToPtr(firstArgRegIdx));
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

    BRANCH(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    BRANCH(IsJSFunction(glue, ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    BRANCH(IsConstructor(glue, ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        BRANCH(IsBase(glue, ctor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            NewObjectStubBuilder newBuilder(this);
            thisObj = newBuilder.FastNewThisObject(glue, ctor);
            BRANCH(HasPendingException(glue), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(
            PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function
        GateRef jumpSize = IntPtr(InterpreterAssembly::GetCallSize(EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8));
        JSCallArgs callArgs(JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV);
        callArgs.callConstructorArgs = { ZExtInt32ToPtr(actualNumArgs), argv, *thisObj };
        CallStubBuilder callBuilder(this, glue, ctor, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs,
            callback);
        res = callBuilder.JSCallDispatch();
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
        BRANCH(TaggedIsException(*res), &isException, &dispatch);
    }
    Bind(&threadCheck);
    {
        BRANCH(HasPendingException(glue), &isException, &dispatch);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&dispatch);
    varAcc = *res;
    DISPATCH_WITH_ACC(WIDE_NEWOBJRANGE_PREF_IMM16_V8);
}

DECLARE_ASM_HANDLER(HandleDefinefuncImm8Id16Imm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef methodId = ReadInst16_1(pc);
    GateRef length = ReadInst8_3(pc);
    GateRef result = DefineFunc(glue, constpool, ZExtInt16ToInt32(methodId));
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    {
        SetLengthToFunction(glue, result, ZExtInt8ToInt32(length));
        auto frame = GetFrame(sp);
        GateRef currentFunc = GetFunctionFromFrame(glue, frame);
        GateRef module = GetModuleFromFunction(glue, currentFunc);
        Label isSendableFunc(env);
        Label isNotSendableFunc(env);
        Label afterSendableFunc(env);
        BRANCH(IsSendableFunction(GetMethodFromFunction(glue, result)), &isSendableFunc, &isNotSendableFunc);
        Bind(&isSendableFunc);
        {
            GateRef smodule = CallRuntime(glue, RTSTUB_ID(GetSharedModule), { module });
            Label isSourceTextModule(env);
            Label isNotSourceTextModule(env);
            BRANCH(IsSourceTextModule(glue, module), &isSourceTextModule, &isNotSourceTextModule);
            Bind(&isSourceTextModule);
            {
                SetSendableEnvToModule(glue, smodule, GetSendableEnvFromModule(glue, module));
                Jump(&isNotSourceTextModule);
            }
            Bind(&isNotSourceTextModule);
            SetModuleToFunction(glue, result, smodule, MemoryAttribute::DefaultWithShareBarrier());
            Jump(&afterSendableFunc);
        }
        Bind(&isNotSendableFunc);
        {
            GateRef envHandle = GetEnvFromFrame(glue, frame);
            SetLexicalEnvToFunction(glue, result, envHandle);
            SetModuleToFunction(glue, result, module);
            SetHomeObjectToFunction(glue, result, GetHomeObjectFromFunction(glue, currentFunc));
#if ECMASCRIPT_ENABLE_IC
            GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
            UpdateProfileTypeInfoCellToFunction(glue, result, profileTypeInfo, slotId);
            callback.ProfileDefineClass(result);
#endif
            Jump(&afterSendableFunc);
        }
        Bind(&afterSendableFunc);
        varAcc = result;
        DISPATCH_WITH_ACC(DEFINEFUNC_IMM8_ID16_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleDefinefuncImm16Id16Imm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef methodId = ReadInst16_2(pc);
    GateRef length = ReadInst8_4(pc);
    GateRef result = DefineFunc(glue, constpool, ZExtInt16ToInt32(methodId));
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    {
        SetLengthToFunction(glue, result, ZExtInt8ToInt32(length));
        auto frame = GetFrame(sp);
        GateRef currentFunc = GetFunctionFromFrame(glue, frame);
        GateRef module = GetModuleFromFunction(glue, currentFunc);
        Label isSendableFunc(env);
        Label isNotSendableFunc(env);
        Label afterSendableFunc(env);
        BRANCH(IsSendableFunction(GetMethodFromFunction(glue, result)), &isSendableFunc, &isNotSendableFunc);
        Bind(&isSendableFunc);
        {
            GateRef smodule = CallRuntime(glue, RTSTUB_ID(GetSharedModule), { module });
            Label isSourceTextModule(env);
            Label isNotSourceTextModule(env);
            BRANCH(IsSourceTextModule(glue, module), &isSourceTextModule, &isNotSourceTextModule);
            Bind(&isSourceTextModule);
            {
                SetSendableEnvToModule(glue, smodule, GetSendableEnvFromModule(glue, module));
                Jump(&isNotSourceTextModule);
            }
            Bind(&isNotSourceTextModule);
            SetModuleToFunction(glue, result, smodule, MemoryAttribute::DefaultWithShareBarrier());
            Jump(&afterSendableFunc);
        }
        Bind(&isNotSendableFunc);
        {
            GateRef envHandle = GetEnvFromFrame(glue, frame);
            SetLexicalEnvToFunction(glue, result, envHandle);
            SetModuleToFunction(glue, result, module);
            SetHomeObjectToFunction(glue, result, GetHomeObjectFromFunction(glue, currentFunc));
#if ECMASCRIPT_ENABLE_IC
            GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
            UpdateProfileTypeInfoCellToFunction(glue, result, profileTypeInfo, slotId);
            callback.ProfileDefineClass(result);
#endif
            Jump(&afterSendableFunc);
        }
        Bind(&afterSendableFunc);
        varAcc = result;
        DISPATCH_WITH_ACC(DEFINEFUNC_IMM16_ID16_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleDefinemethodImm8Id16Imm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef methodId = ReadInst16_1(pc);
    GateRef length = ReadInst8_3(pc);
    GateRef lexEnv = GetEnvFromFrame(glue, GetFrame(sp));
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetMethodFromConstPool(glue, constpool, ZExtInt16ToInt32(methodId)));
    result = CallRuntime(glue, RTSTUB_ID(DefineMethod), { *result, acc, Int8ToTaggedInt(length),
        lexEnv, GetModule(glue, sp) });
#if ECMASCRIPT_ENABLE_IC
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    UpdateProfileTypeInfoCellToFunction(glue, *result, profileTypeInfo, slotId);
#endif
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*result, &notException);
    Bind(&notException);
    {
        varAcc = *result;
        DISPATCH_WITH_ACC(DEFINEMETHOD_IMM8_ID16_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleDefinemethodImm16Id16Imm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef methodId = ReadInst16_2(pc);
    GateRef length = ReadInst8_4(pc);
    GateRef lexEnv = GetEnvFromFrame(glue, GetFrame(sp));
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetMethodFromConstPool(glue, constpool, ZExtInt16ToInt32(methodId)));
    result = CallRuntime(glue, RTSTUB_ID(DefineMethod), { *result, acc, Int8ToTaggedInt(length),
        lexEnv, GetModule(glue, sp) });
#if ECMASCRIPT_ENABLE_IC
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    UpdateProfileTypeInfoCellToFunction(glue, *result, profileTypeInfo, slotId);
#endif
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*result, &notException);
    Bind(&notException);
    {
        varAcc = *result;
        DISPATCH_WITH_ACC(DEFINEMETHOD_IMM16_ID16_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleApplyImm8V8V8)
{
    GateRef func = acc;
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef array = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CallSpread), { func, obj, array });
    CHECK_PENDING_EXCEPTION(res, INT_PTR(APPLY_IMM8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleDeprecatedCallspreadPrefV8V8V8)
{
    GateRef func = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef array = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CallSpread), { func, obj, array });
    CHECK_PENDING_EXCEPTION(res, INT_PTR(DEPRECATED_CALLSPREAD_PREF_V8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleThrowNotexistsPrefNone)
{
    CallRuntime(glue, RTSTUB_ID(ThrowThrowNotExists), {});
    DISPATCH_LAST();
}
DECLARE_ASM_HANDLER(HandleThrowPrefNone)
{
    CallRuntime(glue, RTSTUB_ID(Throw), { acc });
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleJnstricteqV8Imm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJnstricteqV8Imm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJstricteqV8Imm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJstricteqV8Imm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJneV8Imm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJneV8Imm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJeqV8Imm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJeqV8Imm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJnstrictequndefinedImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJnstrictequndefinedImm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJstrictequndefinedImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJstrictequndefinedImm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJneundefinedImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJneundefinedImm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJnstricteqnullImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJnstricteqnullImm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJstricteqnullImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJstricteqnullImm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJnenullImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJnenullImm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJeqnullImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJeqnullImm8)
{
    DISPATCH(NOP);
}

DECLARE_ASM_HANDLER(HandleJnstricteqzImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJnstricteqzImm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJstricteqzImm16)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleJstricteqzImm8)
{
    DISPATCH(NOP);
}
DECLARE_ASM_HANDLER(HandleStthisbyvalueImm16V8)
{
    GateRef v0 = ReadInst8_2(pc);
    GateRef receiver = GetThisFromFrame(glue, GetFrame(sp));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef value = acc;
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, value, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STTHISBYVALUE_IMM16_V8));
}
DECLARE_ASM_HANDLER(HandleStthisbyvalueImm8V8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetThisFromFrame(glue, GetFrame(sp));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef value = acc;
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.StoreObjByValue(glue, receiver, propKey, value, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STTHISBYVALUE_IMM8_V8));
}
DECLARE_ASM_HANDLER(HandleLdthisbyvalueImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef receiver = GetThisFromFrame(glue, GetFrame(sp));
    GateRef propKey = acc;
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.LoadObjByValue(glue, receiver, propKey, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDTHISBYVALUE_IMM16));
}
DECLARE_ASM_HANDLER(HandleLdthisbyvalueImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef receiver = GetThisFromFrame(glue, GetFrame(sp));
    GateRef propKey = acc;
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    AccessObjectStubBuilder builder(this);
    GateRef result = builder.LoadObjByValue(glue, receiver, propKey, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDTHISBYVALUE_IMM8));
}
DECLARE_ASM_HANDLER(HandleStthisbynameImm16Id16)
{
    GateRef receiver = GetThisFromFrame(glue, GetFrame(sp));
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));

    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_2, StringIdInfo::Length::BITS_16);
    GateRef result = builder.StoreObjByName(glue, receiver, 0, info, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STTHISBYNAME_IMM16_ID16));
}
DECLARE_ASM_HANDLER(HandleStthisbynameImm8Id16)
{
    GateRef receiver = GetThisFromFrame(glue, GetFrame(sp));
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));

    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_1, StringIdInfo::Length::BITS_16);
    GateRef result = builder.StoreObjByName(glue, receiver, 0, info, acc, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION(result, INT_PTR(STTHISBYNAME_IMM8_ID16));
}
DECLARE_ASM_HANDLER(HandleLdthisbynameImm16Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
    GateRef receiver = GetThisFromFrame(glue, GetFrame(sp));

    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_2, StringIdInfo::Length::BITS_16);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDTHISBYNAME_IMM16_ID16));
}
DECLARE_ASM_HANDLER(HandleLdthisbynameImm8Id16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef receiver = GetThisFromFrame(glue, GetFrame(sp));

    AccessObjectStubBuilder builder(this);
    StringIdInfo info(constpool, pc, StringIdInfo::Offset::BYTE_1, StringIdInfo::Length::BITS_16);
    GateRef result = builder.LoadObjByName(glue, receiver, 0, info, profileTypeInfo, slotId, callback);
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(LDTHISBYNAME_IMM8_ID16));
}
DECLARE_ASM_HANDLER(HandleLdthis)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = GetThisFromFrame(glue, GetFrame(sp));
    DISPATCH_WITH_ACC(LDTHIS);
}
DECLARE_ASM_HANDLER(HandleLdnewtarget)
{
    DISPATCH(NOP);
}

DECLARE_ASM_HANDLER(HandleDeprecatedLdlexenvPrefNone)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef state = GetFrame(sp);
    varAcc = GetEnvFromFrame(glue, state);
    DISPATCH_WITH_ACC(DEPRECATED_LDLEXENV_PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleDeprecatedLdhomeobjectPrefNone)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef state = GetFunctionFromFrame(glue, GetFrame(sp));
    varAcc = GetHomeObjectFromJSFunction(glue, state);
    DISPATCH_WITH_ACC(DEPRECATED_LDHOMEOBJECT_PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleDeprecatedCreateobjecthavingmethodPrefImm16)
{
    GateRef imm = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef result = GetObjectLiteralFromConstPool(glue, constpool, imm, module);
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.CreateObjectHavingMethod(glue, result, acc);
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16));
}

#define DECLARE_UNUSED_ASM_HANDLE(name)                           \
    DECLARE_ASM_HANDLER(name)                                     \
    {                                                             \
        FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(name)) }); \
        DISPATCH_BAK(OFFSET, IntPtr(0));                          \
    }
ASM_UNUSED_BC_STUB_LIST(DECLARE_UNUSED_ASM_HANDLE)
#undef DECLARE_UNUSED_ASM_HANDLE

DECLARE_ASM_HANDLER_NOPRINT(HandleThrow)
{
    GateRef opcode = ZExtInt8ToPtr(ReadInst8_0(pc));
    auto index = IntPtr(kungfu::BytecodeStubCSigns::ID_Throw_Start);
    auto jumpIndex = PtrAdd(opcode, index);
    DispatchWithId(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, jumpIndex);
}

DECLARE_ASM_HANDLER_NOPRINT(HandleWide)
{
    GateRef opcode = ZExtInt8ToPtr(ReadInst8_0(pc));
    auto index = IntPtr(kungfu::BytecodeStubCSigns::ID_Wide_Start);
    auto jumpIndex = PtrAdd(opcode, index);
    DispatchWithId(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, jumpIndex);
}

DECLARE_ASM_HANDLER_NOPRINT(HandleDeprecated)
{
    GateRef opcode = ZExtInt8ToPtr(ReadInst8_0(pc));
    auto index = IntPtr(kungfu::BytecodeStubCSigns::ID_Deprecated_Start);
    auto jumpIndex = PtrAdd(opcode, index);
    DispatchWithId(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, jumpIndex);
}

DECLARE_ASM_HANDLER_NOPRINT(HandleCallRuntime)
{
    GateRef opcode = ZExtInt8ToPtr(ReadInst8_0(pc));
    auto index = IntPtr(kungfu::BytecodeStubCSigns::ID_CallRuntime_Start);
    auto jumpIndex = PtrAdd(opcode, index);
    DispatchWithId(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, jumpIndex);
}

// interpreter helper handler
DECLARE_ASM_HANDLER_NOPRINT(ExceptionHandler)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label pcIsInvalid(env);
    Label pcNotInvalid(env);
    GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env->IsArch32Bit()));
    GateRef exception = Load(VariableType::JS_ANY(), glue, glue, exceptionOffset);
    varPc = TaggedCastToIntPtr(CallRuntime(glue, RTSTUB_ID(UpFrame), {}));
    varSp = GetCurrentFrame(glue);
    BRANCH(IntPtrEqual(*varPc, IntPtr(0)), &pcIsInvalid, &pcNotInvalid);
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
        GateRef function = GetFunctionFromFrame(glue, GetFrame(*varSp));
        GateRef method = Load(VariableType::JS_ANY(), glue, function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varConstpool = GetConstpoolFromMethod(glue, method);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, function);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeCaughtFrameAndDispatch), {
            glue, *varSp, *varPc, *varConstpool,
            *varProfileTypeInfo, *varAcc, *varHotnessCounter});
        Return();
    }
}

DECLARE_ASM_HANDLER(SingleStepDebugging)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    GateRef frame = GetFrame(*varSp);
    SetPcToFrame(glue, frame, *varPc);
    GateRef currentSp = *varSp;
    varSp = TaggedCastToIntPtr(CallRuntime(glue,
                                           RTSTUB_ID(JumpToCInterpreter),
                                           { constpool, profileTypeInfo, acc,
                                             IntToTaggedInt(hotnessCounter)}));
    GateRef frameAfter = GetFrame(*varSp);
    varPc = GetPcFromFrame(frameAfter);
    Label shouldReturn(env);
    Label shouldContinue(env);

    BRANCH(IntPtrEqual(*varPc, IntPtr(0)), &shouldReturn, &shouldContinue);
    Bind(&shouldReturn);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { Undefined(), *varSp, currentSp });
        Return();
    }
    Bind(&shouldContinue);
    {
        varAcc = GetAccFromFrame(glue, frameAfter);
        GateRef function = GetFunctionFromFrame(glue, frameAfter);
        GateRef method = Load(VariableType::JS_ANY(), glue, function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, function);
        varConstpool = GetConstpoolFromMethod(glue, method);
        varHotnessCounter = GetHotnessCounterFromMethod(method);
    }
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(*varAcc), &isException, &notException);
    Bind(&isException);
    DispatchLast(glue, *varSp, *varPc, *varConstpool, *varProfileTypeInfo, *varAcc,
                 *varHotnessCounter);
    Bind(&notException);
    DISPATCH_BAK(SSD, IntPtr(0));
}

DECLARE_ASM_HANDLER(BCDebuggerEntry)
{
    auto env = GetEnvironment();
    Label callByteCodeChanged(env);
    Label isFrameDroppedTrue(env);
    Label isFrameDroppedFalse(env);
    Label isEntryFrameDroppedPending(env);
    Label isEntryFrameDroppedNotTrue(env);
    Label isBaselineBuiltinFrame(env);
    Label notBaselineBuiltinFrame(env);
    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label pcEqualBaseline(env);
    Label pcNotEqualBaseline(env);
    GateRef frame = GetFrame(sp);
    GateRef isEntryFrameDropped = LoadPrimitive(VariableType::INT8(), glue,
        IntPtr(JSThread::GlueData::GetEntryFrameDroppedStateOffset(env->Is32Bit())));
    BRANCH(Int8Equal(isEntryFrameDropped, Int8(JSThread::FrameDroppedState::StatePending)),
        &isEntryFrameDroppedPending, &callByteCodeChanged);
    Bind(&isEntryFrameDroppedPending);
    {
        Store(VariableType::INT8(), glue, glue,
            IntPtr(JSThread::GlueData::GetEntryFrameDroppedStateOffset(env->Is32Bit())),
            Int8(JSThread::FrameDroppedState::StateFalse));
        DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
        DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
        varPc = GetPcFromFrame(frame);
        varAcc = GetAccFromFrame(glue, frame);
        Dispatch(glue, sp, *varPc, constpool, profileTypeInfo, *varAcc, hotnessCounter, IntPtr(0));
    }
    Bind(&callByteCodeChanged);
    SetPcToFrame(glue, frame, pc);
    // NOTIFY_DEBUGGER_EVENT()
    CallRuntime(glue, RTSTUB_ID(NotifyBytecodePcChanged), {});
    GateRef isFrameDropped = LoadPrimitive(VariableType::BOOL(), glue,
        IntPtr(JSThread::GlueData::GetIsFrameDroppedOffset(env->Is32Bit())));
    BRANCH(isFrameDropped, &isFrameDroppedTrue, &isFrameDroppedFalse);
    Bind(&isFrameDroppedTrue);
    {
        DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
        DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
        DEFVARIABLE(prevState, VariableType::NATIVE_POINTER(), sp);
        DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
        DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
        DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
        DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);
        GateRef state = GetFrame(*varSp);
        GateRef currentSp = *varSp;
        Store(VariableType::BOOL(), glue, glue,
            IntPtr(JSThread::GlueData::GetIsFrameDroppedOffset(env->Is32Bit())), False());
        varSp = LoadPrimitive(VariableType::NATIVE_POINTER(), state,
            IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
        isEntryFrameDropped = LoadPrimitive(VariableType::INT8(), glue,
            IntPtr(JSThread::GlueData::GetEntryFrameDroppedStateOffset(env->Is32Bit())));
        BRANCH(Int8Equal(isEntryFrameDropped, Int8(JSThread::FrameDroppedState::StateTrue)),
            &pcEqualNullptr, &isEntryFrameDroppedNotTrue);
        Bind(&isEntryFrameDroppedNotTrue);
        GateRef typePos = PtrSub(*varSp, IntPtr(JSTaggedValue::TaggedTypeSize()));
        GateRef maybeFrameType = LoadZeroOffsetPrimitive(VariableType::INT64(), typePos);
        BRANCH(Int64Equal(maybeFrameType, Int64(static_cast<int64_t>(FrameType::BASELINE_BUILTIN_FRAME))),
               &isBaselineBuiltinFrame, &notBaselineBuiltinFrame);
        Bind(&isBaselineBuiltinFrame);
        {
            varSp = LoadZeroOffsetPrimitive(VariableType::NATIVE_POINTER(), *varSp);
            Jump(&notBaselineBuiltinFrame);
        }
        Bind(&notBaselineBuiltinFrame);
        prevState = GetFrame(*varSp);
        varPc = GetPcFromFrame(*prevState);
        BRANCH(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
        Bind(&pcEqualNullptr);
        {
            CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc, *varSp, currentSp });
            Return();
        }
        Bind(&pcNotEqualNullptr);
        BRANCH(IntPtrEqual(*varPc, IntPtr(BASELINEJIT_PC_FLAG)), &pcEqualBaseline, &pcNotEqualBaseline);
        Bind(&pcEqualBaseline);
        {
            CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc, *varSp, currentSp });
            Return();
        }
        Bind(&pcNotEqualBaseline);
        {
            GateRef function = GetFunctionFromFrame(glue, *prevState);
            GateRef method = Load(VariableType::JS_ANY(), glue, function, IntPtr(JSFunctionBase::METHOD_OFFSET));
            varConstpool = GetConstpoolFromMethod(glue, method);
            varProfileTypeInfo = GetProfileTypeInfoFromFunction(glue, function);
            varHotnessCounter = GetHotnessCounterFromMethod(method);
            GateRef jumpSize = IntPtr(0);
            CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndRollback),
                { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
                *varAcc, *varHotnessCounter, jumpSize });
            Return();
        }
    }
    Bind(&isFrameDroppedFalse);
    SetAccToFrame(glue, frame, acc);
    // goto normal handle stub
    DispatchDebugger(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
}

DECLARE_ASM_HANDLER(BCDebuggerExceptionEntry)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = Hole();
    GateRef frame = GetFrame(sp);
    SetPcToFrame(glue, frame, pc);
    // NOTIFY_DEBUGGER_EVENT()
    CallRuntime(glue, RTSTUB_ID(NotifyBytecodePcChanged), {});
    // goto last handle stub
    DispatchDebuggerLast(glue, sp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter);
}

DECLARE_ASM_HANDLER(NewObjectRangeThrowException)
{
    CallRuntime(glue, RTSTUB_ID(ThrowDerivedMustReturnException), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(ThrowStackOverflowException)
{
    CallRuntime(glue, RTSTUB_ID(ThrowStackOverflowException), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleDefinefuncImm8Id16Imm8ColdReload)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef methodId = ReadInst16_1(pc);
    GateRef length = ReadInst8_3(pc);
    GateRef result = DefineFunc(glue, constpool, ZExtInt16ToInt32(methodId));
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    {
        SetLengthToFunction(glue, result, ZExtInt8ToInt32(length));
        auto frame = GetFrame(sp);
        GateRef envHandle = GetEnvFromFrame(glue, frame);
        SetLexicalEnvToFunction(glue, result, envHandle);
        GateRef currentFunc = GetFunctionFromFrame(glue, frame);
        SetModuleToFunction(glue, result, GetModuleFromFunction(glue, currentFunc));
        CallRuntime(glue, RTSTUB_ID(SetPatchModule), { result });
        SetHomeObjectToFunction(glue, result, GetHomeObjectFromFunction(glue, currentFunc));
#if ECMASCRIPT_ENABLE_IC
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        UpdateProfileTypeInfoCellToFunction(glue, result, profileTypeInfo, slotId);
        callback.ProfileDefineClass(result);
#endif
        varAcc = result;
        DISPATCH_WITH_ACC(DEFINEFUNC_IMM8_ID16_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleDefinefuncImm16Id16Imm8ColdReload)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef methodId = ReadInst16_2(pc);
    GateRef length = ReadInst8_4(pc);
    GateRef result = DefineFunc(glue, constpool, ZExtInt16ToInt32(methodId));
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(result, &notException);
    Bind(&notException);
    {
        SetLengthToFunction(glue, result, ZExtInt8ToInt32(length));
        auto frame = GetFrame(sp);
        GateRef envHandle = GetEnvFromFrame(glue, frame);
        SetLexicalEnvToFunction(glue, result, envHandle);
        GateRef currentFunc = GetFunctionFromFrame(glue, frame);
        SetHomeObjectToFunction(glue, result, GetHomeObjectFromFunction(glue, currentFunc));
        SetModuleToFunction(glue, result, GetModuleFromFunction(glue, currentFunc));
        CallRuntime(glue, RTSTUB_ID(SetPatchModule), { result });
#if ECMASCRIPT_ENABLE_IC
        GateRef slotId = ZExtInt16ToInt32(ReadInst16_0(pc));
        UpdateProfileTypeInfoCellToFunction(glue, result, profileTypeInfo, slotId);
        callback.ProfileDefineClass(result);
#endif
        varAcc = result;
        DISPATCH_WITH_ACC(DEFINEFUNC_IMM16_ID16_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleWideLdpatchvarPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef index = ReadInst16_1(pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdPatchVar), { Int16ToTaggedInt(index) });
    CHECK_EXCEPTION_WITH_VARACC(result, INT_PTR(WIDE_LDPATCHVAR_PREF_IMM16));
}

DECLARE_ASM_HANDLER(HandleWideStpatchvarPrefImm16)
{
    GateRef index = ReadInst16_1(pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StPatchVar), { Int16ToTaggedInt(index), acc });
    CHECK_EXCEPTION(result, INT_PTR(WIDE_STPATCHVAR_PREF_IMM16));
}

DECLARE_ASM_HANDLER(HandleCallRuntimeNotifyConcurrentResultPrefNone)
{
    GateRef funcObj = GetFunctionFromFrame(glue, GetFrame(sp));
    CallRuntime(glue, RTSTUB_ID(NotifyConcurrentResult), {acc, funcObj});
    DISPATCH(CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleDefineFieldByNameImm8Id16V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFINE_BY_NAME(Boolean(false));
    CHECK_EXCEPTION_WITH_ACC(*result, INT_PTR(DEFINEFIELDBYNAME_IMM8_ID16_V8));
}

DECLARE_ASM_HANDLER(HandleDefinePropertyByNameImm8Id16V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFINE_BY_NAME(Boolean(true));
    CHECK_EXCEPTION_WITH_ACC(*result, INT_PTR(DEFINEPROPERTYBYNAME_IMM8_ID16_V8));
}

DECLARE_ASM_HANDLER(HandleCallRuntimeDefineFieldByValuePrefImm8V8V8)
{
    GateRef v0 = ReadInst8_2(pc);
    GateRef v1 = ReadInst8_3(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(v1));
    GateRef propKey = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef res = DefineField(glue, obj, propKey, acc);  // acc as value
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8));
}

DECLARE_ASM_HANDLER(HandleCallRuntimeDefineFieldByIndexPrefImm8Imm32V8)
{
    GateRef index = ReadInst32_2(pc);
    GateRef v0 = ReadInst8_6(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef propKey = IntToTaggedPtr(index);
    GateRef res = DefineField(glue, obj, propKey, acc);  // acc as value
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8));
}

DECLARE_ASM_HANDLER(HandleCallRuntimeToPropertyKeyPrefNone)
{
    GateRef res = ToPropertyKey(glue, acc);
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CALLRUNTIME_TOPROPERTYKEY_PREF_NONE));
}

DECLARE_ASM_HANDLER(HandleCallRuntimeCreatePrivatePropertyPrefImm16Id16)
{
    GateRef lexicalEnv = GetEnvFromFrame(glue, GetFrame(sp));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef count = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef literalId = ZExtInt16ToInt32(ReadInst16_3(pc));
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreatePrivateProperty), {lexicalEnv,
        IntToTaggedInt(count), constpool, IntToTaggedInt(literalId), module});
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16));
}

DECLARE_ASM_HANDLER(HandleCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8)
{
    GateRef lexicalEnv = GetEnvFromFrame(glue, GetFrame(sp));
    GateRef levelIndex = ReadInst16_2(pc);
    GateRef slotIndex = ReadInst16_4(pc);
    GateRef v0 = ReadInst8_6(pc);
    GateRef obj = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef res = CallRuntime(glue, RTSTUB_ID(DefinePrivateProperty), {lexicalEnv,
        IntToTaggedInt(levelIndex), IntToTaggedInt(slotIndex), obj, acc});  // acc as value
    CHECK_EXCEPTION_WITH_ACC(res, INT_PTR(CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8));
}

DECLARE_ASM_HANDLER(HandleCallRuntimeCallInitPrefImm8V8)
{
    // same as callthis0
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG0);
    GateRef thisValue = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef func = acc;
    METHOD_ENTRY(func);
    GateRef jumpSize = INT_PTR(CALLRUNTIME_CALLINIT_PREF_IMM8_V8);
    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG0);
    callArgs.callArgsWithThis = { 0, 0, 0, thisValue };
    CallStubBuilder callBuilder(this, glue, func, actualNumArgs, jumpSize, nullptr, hotnessCounter, callArgs, callback);
    GateRef res = callBuilder.JSCallDispatch();
    CHECK_PENDING_EXCEPTION(res, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef methodId = ReadInst16_3(pc);
    GateRef literalId = ReadInst16_5(pc);
    GateRef length = ReadInst16_7(pc);
    GateRef v0 = ReadInst8_9(pc);

    GateRef proto = GetVregValue(glue, sp, ZExtInt8ToPtr(v0));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateSharedClass),
                              { proto, constpool,
                                Int16ToTaggedInt(methodId),
                                Int16ToTaggedInt(literalId),
                                Int16ToTaggedInt(length), module });

    Label isException(env);
    Label isNotException(env);
    BRANCH(TaggedIsException(res), &isException, &isNotException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&isNotException);
    varAcc = res;
    DISPATCH_WITH_ACC(CALLRUNTIME_DEFINESENDABLECLASS_PREF_IMM16_ID16_ID16_IMM16_V8);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeLdSendableClassPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef level = ReadInst16_1(pc);
    GateRef lexEnv = GetEnvFromFrame(glue, GetFrame(sp));
    varAcc = CallRuntime(glue, RTSTUB_ID(LdSendableClass), { lexEnv, Int16ToTaggedInt(level) });
    DISPATCH_WITH_ACC(CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeLdsendableexternalmodulevarImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(moduleRef, VariableType::JS_ANY(), Undefined());
    GateRef index = ReadInst8_1(pc);

    // LdSendableExternalModuleVarByIndex may load uninitialized module lazy. Exception could happened.
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    moduleRef = CallRuntime(glue, RTSTUB_ID(LdSendableExternalModuleVarByIndex), {Int8ToTaggedInt(index), currentFunc});

    auto env = GetEnvironment();
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*moduleRef, &notException);
    Bind(&notException);
    {
        varAcc = *moduleRef;
        DISPATCH_WITH_ACC(CALLRUNTIME_LDSENDABLEEXTERNALMODULEVAR_PREF_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleCallRuntimeWideLdsendableexternalmodulevarPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(moduleRef, VariableType::JS_ANY(), Undefined());

    GateRef index = ReadInst16_1(pc);

    // LdSendableExternalModuleVarByIndex may load uninitialized module lazy. Exception could happened.
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    moduleRef =
        CallRuntime(glue, RTSTUB_ID(LdSendableExternalModuleVarByIndex), {Int16ToTaggedInt(index), currentFunc});

    auto env = GetEnvironment();
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*moduleRef, &notException);
    Bind(&notException);
    {
        varAcc = *moduleRef;
        DISPATCH_WITH_ACC(CALLRUNTIME_WIDELDSENDABLEEXTERNALMODULEVAR_PREF_IMM16);
    }
}

DECLARE_ASM_HANDLER(HandleCallRuntimeNewSendableEnvImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef numVars = ZExtInt8ToInt16(ReadInst8_1(pc));
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewSendableEnv),
                              { Int16ToTaggedInt(numVars) });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
    Bind(&notException);
    varAcc = res;
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    SetSendableEnvToModule(glue, module, res);
    DISPATCH_WITH_ACC(CALLRUNTIME_NEWSENDABLEENV_PREF_IMM8);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeNewSendableEnvImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef numVars = ReadInst16_1(pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewSendableEnv),
                              { Int16ToTaggedInt(numVars) });
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(res, &notException);
    Bind(&notException);
    varAcc = res;
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    SetSendableEnvToModule(glue, module, res);
    DISPATCH_WITH_ACC(CALLRUNTIME_WIDENEWSENDABLEENV_PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeStSendableVarImm4Imm4)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt8ToInt32(ReadInst4_2(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_3(pc));

    GateRef value = acc;
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetSendableEnvFromModule(glue, module));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetSendableParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToSendableEnv(glue, *currentEnv, slot, value);
    DISPATCH(CALLRUNTIME_STSENDABLEVAR_PREF_IMM4_IMM4);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeStSendableVarImm8Imm8)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_2(pc));

    GateRef value = acc;
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetSendableEnvFromModule(glue, module));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetSendableParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToSendableEnv(glue, *currentEnv, slot, value);
    DISPATCH(CALLRUNTIME_STSENDABLEVAR_PREF_IMM8_IMM8);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeStSendableVarImm16Imm16)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));

    GateRef value = acc;
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetSendableEnvFromModule(glue, module));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetSendableParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    SetPropertiesToSendableEnv(glue, *currentEnv, slot, value);
    DISPATCH(CALLRUNTIME_WIDESTSENDABLEVAR_PREF_IMM16_IMM16);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeLdSendableVarImm4Imm4)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt8ToInt32(ReadInst4_2(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_3(pc));
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetSendableEnvFromModule(glue, module));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetSendableParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromSendableEnv(glue, *currentEnv, slot);
    varAcc = variable;

    DISPATCH_WITH_ACC(CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeLdSendableVarImm8Imm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_2(pc));

    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetSendableEnvFromModule(glue, module));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetSendableParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromSendableEnv(glue, *currentEnv, slot);
    varAcc = variable;

    DISPATCH_WITH_ACC(CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8);
}

DECLARE_ASM_HANDLER(HandleCallRuntimeLdSendableVarImm16Imm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));

    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    GateRef module = GetModuleFromFunction(glue, currentFunc);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetSendableEnvFromModule(glue, module));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetSendableParentEnv(glue, *currentEnv);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromSendableEnv(glue, *currentEnv, slot);
    varAcc = variable;

    DISPATCH_WITH_ACC(CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16);
}
DECLARE_ASM_HANDLER(HandleCallRuntimeLdLazyModuleVarPrefImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(moduleRef, VariableType::JS_ANY(), Undefined());
    GateRef index = ReadInst8_1(pc);

    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    moduleRef = CallRuntime(glue, RTSTUB_ID(LdLazyExternalModuleVarByIndex), { Int8ToTaggedInt(index), currentFunc });

    auto env = GetEnvironment();
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*moduleRef, &notException);
    Bind(&notException);
    {
        varAcc = *moduleRef;
        DISPATCH_WITH_ACC(CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleCallRuntimeWideLdLazyModuleVarPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(moduleRef, VariableType::JS_ANY(), Undefined());

    GateRef index = ReadInst16_1(pc);

    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    moduleRef = CallRuntime(glue, RTSTUB_ID(LdLazyExternalModuleVarByIndex), { Int16ToTaggedInt(index), currentFunc });

    auto env = GetEnvironment();
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*moduleRef, &notException);
    Bind(&notException);
    {
        varAcc = *moduleRef;
        DISPATCH_WITH_ACC(CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16);
    }
}

DECLARE_ASM_HANDLER(HandleCallRuntimeLdLazySendableModuleVarPrefImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(moduleRef, VariableType::JS_ANY(), Undefined());
    GateRef index = ReadInst8_1(pc);

    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    moduleRef = CallRuntime(glue, RTSTUB_ID(LdLazySendableExternalModuleVarByIndex),
        { Int8ToTaggedInt(index), currentFunc });

    auto env = GetEnvironment();
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*moduleRef, &notException);
    Bind(&notException);
    {
        varAcc = *moduleRef;
        DISPATCH_WITH_ACC(CALLRUNTIME_LDLAZYSENDABLEMODULEVAR_PREF_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleCallRuntimeWideLdLazySendableModuleVarPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(moduleRef, VariableType::JS_ANY(), Undefined());

    GateRef index = ReadInst16_1(pc);

    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    moduleRef = CallRuntime(glue, RTSTUB_ID(LdLazySendableExternalModuleVarByIndex),
        { Int16ToTaggedInt(index), currentFunc });

    auto env = GetEnvironment();
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*moduleRef, &notException);
    Bind(&notException);
    {
        varAcc = *moduleRef;
        DISPATCH_WITH_ACC(CALLRUNTIME_WIDELDLAZYSENDABLEMODULEVAR_PREF_IMM16);
    }
}

DECLARE_ASM_HANDLER(HandleCallRuntimeSuperCallForwardAllArgsV8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    auto env = GetEnvironment();
    Label resIsException(env);
    Label isException(env);
    Label dispatch(env);

    GateRef thisFunc = GetVregValue(glue, sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    res = CallRuntime(glue, RTSTUB_ID(SuperCallForwardAllArgs), { thisFunc });

    BRANCH(TaggedIsException(*res), &resIsException, &dispatch);
    Bind(&resIsException);
    BRANCH(HasPendingException(glue), &isException, &dispatch);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&dispatch);
    {
        varAcc = *res;
        DISPATCH_WITH_ACC(CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8);
    }
}

DECLARE_ASM_HANDLER(HandleCallRuntimeLdsendablelocalmodulevarImm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(moduleRef, VariableType::JS_ANY(), Undefined());
    GateRef index = ReadInst8_1(pc);

    // LdSendableLocalModuleVarByIndex may load uninitialized module lazy. Exception could happened.
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    moduleRef = CallRuntime(glue, RTSTUB_ID(LdSendableLocalModuleVarByIndex), {Int8ToTaggedInt(index), currentFunc});

    auto env = GetEnvironment();
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*moduleRef, &notException);
    Bind(&notException);
    {
        varAcc = *moduleRef;
        DISPATCH_WITH_ACC(CALLRUNTIME_LDSENDABLELOCALMODULEVAR_PREF_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleCallRuntimeWideLdsendablelocalmodulevarPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(moduleRef, VariableType::JS_ANY(), Undefined());

    GateRef index = ReadInst16_1(pc);

    // LdSendableLocalModuleVarByIndex may load uninitialized module lazy. Exception could happened.
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    moduleRef =
        CallRuntime(glue, RTSTUB_ID(LdSendableLocalModuleVarByIndex), {Int16ToTaggedInt(index), currentFunc});

    auto env = GetEnvironment();
    Label notException(env);
    CHECK_EXCEPTION_WITH_JUMP(*moduleRef, &notException);
    Bind(&notException);
    {
        varAcc = *moduleRef;
        DISPATCH_WITH_ACC(CALLRUNTIME_WIDELDSENDABLELOCALMODULEVAR_PREF_IMM16);
    }
}

ASM_INTERPRETER_BC_TYPE_PROFILER_STUB_LIST(DECLARE_ASM_HANDLER_PROFILE)
ASM_INTERPRETER_BC_LAYOUT_PROFILER_STUB_LIST(DECLARE_ASM_HANDLER_PROFILE)
ASM_INTERPRETER_BC_FUNC_HOT_PROFILER_STUB_LIST(DECLARE_ASM_HANDLER_PROFILE)
ASM_INTERPRETER_BC_FUNC_COUNT_PROFILER_STUB_LIST(DECLARE_ASM_HANDLER_PROFILE)
ASM_INTERPRETER_BC_FUNC_HOT_JIT_PROFILER_STUB_LIST(DECLARE_ASM_HANDLER_JIT_PROFILE)

#undef DECLARE_ASM_HANDLER
#undef DISPATCH
#undef DISPATCH_WITH_ACC
#undef DISPATCH_LAST
#undef DISPATCH_LAST_WITH_ACC
#undef USE_PARAMS
}  // namespace panda::ecmascript::kungfu
