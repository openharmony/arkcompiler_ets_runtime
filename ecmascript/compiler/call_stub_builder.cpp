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

#include "ecmascript/compiler/call_stub_builder.h"

#include <cstddef>
#include <tuple>

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/slowpath_lowering.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {

void CallStubBuilder::JSCallDispatchForBaseline(Label *exit, Label *noNeedCheckException)
{
    this->isForBaseline_ = true;
    auto env = GetEnvironment();
    baselineBuiltinFp_ = CallNGCRuntime(glue_, RTSTUB_ID(GetBaselineBuiltinFp), {glue_});

    CallNGCRuntime(glue_, RTSTUB_ID(CopyCallTarget), { glue_, func_ });
    if (callArgs_.mode == JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV) {
        CallNGCRuntime(glue_, RTSTUB_ID(CopyArgvArray),
            { glue_, callArgs_.superCallArgs.argv, callArgs_.superCallArgs.argc });
    }

    // 1. call initialize
    Label funcIsHeapObject(env);
    Label funcIsCallable(env);
    Label funcNotCallable(env);
    JSCallInit(exit, &funcIsHeapObject, &funcIsCallable, &funcNotCallable);

    // 2. dispatch
    Label methodIsNative(env);
    Label methodNotNative(env);
    BRANCH(Int64NotEqual(Int64And(callField_, isNativeMask_), Int64(0)), &methodIsNative, &methodNotNative);

    // 3. call native
    Bind(&methodIsNative);
    {
        JSCallNative(exit);
    }

    // 4. call nonNative
    Bind(&methodNotNative);
    {
        JSCallJSFunction(exit, noNeedCheckException);
    }
}

std::tuple<bool, size_t, size_t> CallCoStubBuilder::GetOpInfo()
{
    switch (op_) {
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
            return std::make_tuple(true, 2, 0);        // needThis: true, numberValueIn: 2, numberArg: 0
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            return std::make_tuple(true, 3, 1);        // needThis: true, numberValueIn: 3, numberArg: 1
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            return std::make_tuple(true, 4, 2);	       // needThis: true, numberValueIn: 4, numberArg: 2
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            return std::make_tuple(true, 5, 3);        // needThis: true, numberValueIn: 5, numberArg: 3
        case EcmaOpcode::CALLARG0_IMM8:
            return std::make_tuple(false, 1, 0);       // needThis: false, numberValueIn: 1, numberArg: 0
        case EcmaOpcode::CALLARG1_IMM8_V8:
            return std::make_tuple(false, 2, 1);       // needThis: false, numberValueIn: 2, numberArg: 1
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
            return std::make_tuple(false, 3, 2);       // needThis: false, numberValueIn: 3, numberArg: 2
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
            return std::make_tuple(false, 4, 3);       // needThis: false, numberValueIn: 4, numberArg: 3
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void CallCoStubBuilder::PrepareArgs(std::vector<GateRef> &args, std::vector<GateRef> &argsFastCall)
{
    auto env = GetEnvironment();
    auto &builder =  *env->GetBuilder();
    auto [needThis, numberValueIn, numberArg] = GetOpInfo();
    actualArgc_ = builder.Int64(BytecodeCallArgc::ComputeCallArgc(numberValueIn, op_));
    actualArgv_ = builder.IntPtr(0);
    newTarget_ = builder.Undefined();
    thisObj_ = builder.Undefined();
    hirGate_ = Circuit::NullGate();

    size_t argIndex = 0;
    glue_ = PtrArgument(argIndex++);
    func_ = TaggedArgument(argIndex++);

    // keep same with CommonArgIdx
    args.push_back(glue_);
    args.push_back(actualArgc_);
    args.push_back(actualArgv_);
    args.push_back(func_);
    args.push_back(newTarget_);

    // keep same with FastCallArgIdx
    argsFastCall.push_back(glue_);
    argsFastCall.push_back(func_);

    // solve thisobj
    if (needThis) {
        thisObj_ = TaggedArgument(argIndex++);
    }
    args.push_back(thisObj_);
    argsFastCall.push_back(thisObj_);

    // solve args
    for (size_t i = 0; i < numberArg; i++) {
        args.push_back(TaggedArgument(argIndex));
        argsFastCall.push_back(TaggedArgument(argIndex));
        argIndex++;
    }
}

void CallCoStubBuilder::FastCallSelector(CircuitBuilder &builder, GateRef glue, GateRef func, GateRef argc,
                                         Variable *result, Label *exit)
{
    Label isHeapJSFunction(&builder);
    Label fastCall(&builder);
    Label notFastCall(&builder);
    Label call(&builder);
    Label call1(&builder);
    Label slowCall(&builder);
    Label callBridge(&builder);
    Label callBridge1(&builder);
    // use builder_ to make BRANCH_CIR work.
    auto &builder_ = builder;
    BRANCH_CIR(builder.TaggedIsJSFunction(glue, func), &isHeapJSFunction, exit);
    builder.Bind(&isHeapJSFunction);
    {
        GateRef method = builder.GetMethodFromFunction(glue, func);
        GateRef expectedArgc =
            builder.Int64Add(builder.GetExpectedNumOfArgs(method), builder.Int64(NUM_MANDATORY_JSFUNC_ARGS));
        BRANCH_CIR(builder.JudgeAotAndFastCall(func, CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL), &fastCall,
                   &notFastCall);
        builder.Bind(&fastCall);
        {
            BRANCH_CIR(builder.Equal(expectedArgc, argc), &call, &callBridge);
            builder.Bind(&call);
            result->WriteVariable(builder.Int32(static_cast<int32_t>(FastCallType::FAST_AOT_CALL)));
            builder.Jump(exit);
            builder.Bind(&callBridge);
            result->WriteVariable(builder.Int32(static_cast<int32_t>(FastCallType::FAST_AOT_CALL_BRIDGE)));
            builder.Jump(exit);
        }
        builder.Bind(&notFastCall);
        BRANCH_CIR(builder.JudgeAotAndFastCall(func, CircuitBuilder::JudgeMethodType::HAS_AOT), &slowCall, exit);
        builder.Bind(&slowCall);
        {
            BRANCH_CIR(builder.Equal(expectedArgc, argc), &call1, &callBridge1);
            builder.Bind(&call1);
            result->WriteVariable(builder.Int32(static_cast<int32_t>(FastCallType::AOT_CALL)));
            builder.Jump(exit);
            builder.Bind(&callBridge1);
            result->WriteVariable(builder.Int32(static_cast<int32_t>(FastCallType::AOT_CALL_BRIDGE)));
            builder.Jump(exit);
        }
    }
}

GateRef CallCoStubBuilder::LowerCallNGCRuntime(GateRef glue, CircuitBuilder &builder, GateRef gate, int index,
                                               const std::vector<GateRef> &args, bool useLabel)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    if (useLabel) {
        GateRef result = builder.CallNGCRuntime(glue, index, Gate::InvalidGateRef, args, gate, name.c_str());
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(index);
        GateRef target = builder.IntPtr(index);
        GateRef result = builder.Call(cs, glue, target, builder.GetDepend(), args, gate, name.c_str());
        return result;
    }
}

void CallCoStubBuilder::CallNGCRuntimeWithCallTimer(GateRef glue, CircuitBuilder &builder, int index, GateRef gate,
                                                    GateRef func, Variable &result, const std::vector<GateRef> &args)
{
    builder.StartCallTimer(glue, gate, {glue, func, builder.True()}, true);
    result = LowerCallNGCRuntime(glue, builder, gate, index, args, true);
    builder.EndCallTimer(glue, gate, {glue, func}, true);
}

static GateRef IsAotFastCall(CircuitBuilder &builder, GateRef func)
{
    return builder.JudgeAotAndFastCall(func, CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL);
}

static GateRef IsAotCall(CircuitBuilder &builder, GateRef func)
{
    return builder.JudgeAotAndFastCall(func, CircuitBuilder::JudgeMethodType::HAS_AOT);
}

void CallCoStubBuilder::LowerFastSuperCall(GateRef glue, CircuitBuilder &builder, const std::vector<GateRef> &args,
                                           GateRef elementsPtr, Variable &result, Label &exit)
{
    auto &builder_ = builder;
    Label fastCall(&builder);
    Label notFastCall(&builder);
    Label aotCall(&builder);
    Label notAotCall(&builder);
    ASSERT(args.size() == 5);     // 5 : size of args
    GateRef gate = args[0];       // 0 : index of gate
    GateRef superFunc = args[1];  // 1 : index of superFunc
    GateRef newTarget = args[2];  // 2 : index of newTarget
    GateRef thisObj = args[3];    // 3 : index of thisObj
    GateRef actualArgc = args[4]; // 4 : index of actualArgc

    GateRef method = builder.GetMethodFromFunction(glue, superFunc);
    GateRef expectedNum = builder.GetExpectedNumOfArgs(method);
#ifdef USE_READ_BARRIER
    builder_.CallNGCRuntime(glue, RTSTUB_ID(CopyCallTarget),
                            Gate::InvalidGateRef, {glue, superFunc}, glue);
    builder_.CallNGCRuntime(glue, RTSTUB_ID(CopyArgvArray),
                            Gate::InvalidGateRef,
                            {glue, elementsPtr, actualArgc}, glue);
#endif
    BRANCH_CIR(IsAotFastCall(builder, superFunc), &fastCall, &notFastCall);
    builder.Bind(&fastCall);
    {
        Label notBridge(&builder);
        Label bridge(&builder);
        BRANCH_CIR(builder.Int64Equal(expectedNum, actualArgc), &notBridge, &bridge);
        builder.Bind(&notBridge);
        CallNGCRuntimeWithCallTimer(glue, builder, RTSTUB_ID(JSFastCallWithArgV), gate, superFunc, result,
                                    {glue, superFunc, thisObj, actualArgc, elementsPtr});
        builder.Jump(&exit);
        builder.Bind(&bridge);
        CallNGCRuntimeWithCallTimer(glue, builder, RTSTUB_ID(JSFastCallWithArgVAndPushArgv), gate, superFunc, result,
                                    {glue, superFunc, thisObj, actualArgc, elementsPtr, expectedNum});
        builder.Jump(&exit);
    }
    builder.Bind(&notFastCall);
    BRANCH_CIR(IsAotCall(builder, superFunc), &aotCall, &notAotCall);
    builder_.Bind(&aotCall);
    {
        Label notBridge(&builder);
        Label bridge(&builder);
        std::vector<GateRef> callArgs {glue, actualArgc, superFunc, newTarget, thisObj, elementsPtr};
        BRANCH_CIR(builder.Int64Equal(expectedNum, actualArgc), &notBridge, &bridge);
        builder.Bind(&notBridge);
        CallNGCRuntimeWithCallTimer(glue, builder, RTSTUB_ID(JSCallWithArgV), gate, superFunc, result, callArgs);
        builder.Jump(&exit);
        builder.Bind(&bridge);
        CallNGCRuntimeWithCallTimer(glue, builder, RTSTUB_ID(JSCallWithArgVAndPushArgv), gate, superFunc, result,
                                    callArgs);
        builder.Jump(&exit);
    }
    builder.Bind(&notAotCall);
    CallNGCRuntimeWithCallTimer(glue, builder, RTSTUB_ID(SuperCallWithArgV), gate, superFunc, result,
                                {glue, actualArgc, superFunc, newTarget, thisObj, elementsPtr});
    builder.Jump(&exit);
}

void CallCoStubBuilder::LowerFastCall(GateRef gate, GateRef glue, CircuitBuilder &builder, GateRef func, GateRef argc,
    const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall,
    Variable *result, Label *exit, bool isNew)
{
    Label isHeapObject(&builder);
    Label isJsFcuntion(&builder);
    Label fastCall(&builder);
    Label notFastCall(&builder);
    Label call(&builder);
    Label call1(&builder);
    Label slowCall(&builder);
    Label callBridge(&builder);
    Label callBridge1(&builder);
    Label slowPath(&builder);
    Label notCallConstructor(&builder);
    Label isCallConstructor(&builder);
    // use builder_ to make BRANCH_CIR work.
    auto &builder_ = builder;
#ifdef USE_READ_BARRIER
    builder_.CallNGCRuntime(glue, RTSTUB_ID(CopyCallTarget),
                            Gate::InvalidGateRef, {glue, func}, glue);
#endif
    BRANCH_CIR(builder.TaggedIsHeapObject(func), &isHeapObject, &slowPath);
    builder.Bind(&isHeapObject);
    {
        BRANCH_CIR(builder.IsJSFunction(glue, func), &isJsFcuntion, &slowPath);
        builder.Bind(&isJsFcuntion);
        {
            if (!isNew) {
                BRANCH_CIR(builder.IsClassConstructor(glue, func), &slowPath, &notCallConstructor);
                builder.Bind(&notCallConstructor);
            }
            GateRef method = builder.GetMethodFromFunction(glue, func);
            BRANCH_CIR(builder.JudgeAotAndFastCall(func,
                CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL), &fastCall, &notFastCall);
            builder.Bind(&fastCall);
            {
                GateRef expectedArgc = builder.Int64Add(builder.GetExpectedNumOfArgs(method),
                    builder.Int64(NUM_MANDATORY_JSFUNC_ARGS));
                BRANCH_CIR(builder.Equal(expectedArgc, argc), &call, &callBridge);
                builder.Bind(&call);
                {
                    builder.StartCallTimer(glue, gate, {glue, func, builder.True()}, true);
                    GateRef code = builder.GetCodeAddr(func);
                    auto depend = builder.GetDepend();
                    const CallSignature *cs = RuntimeStubCSigns::GetOptimizedFastCallSign();
                    result->WriteVariable(builder.Call(cs, glue, code, depend, argsFastCall, gate, "callFastAOT"));
                    builder.EndCallTimer(glue, gate, {glue, func}, true);
                    builder.Jump(exit);
                }
                builder.Bind(&callBridge);
                {
                    builder.StartCallTimer(glue, gate, {glue, func, builder.True()}, true);
                    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(OptimizedFastCallAndPushArgv));
                    GateRef target = builder.IntPtr(RTSTUB_ID(OptimizedFastCallAndPushArgv));
                    auto depend = builder.GetDepend();
                    result->WriteVariable(builder.Call(cs, glue, target, depend, args, gate, "callFastBridge"));
                    builder.EndCallTimer(glue, gate, {glue, func}, true);
                    builder.Jump(exit);
                }
            }
            builder.Bind(&notFastCall);
            BRANCH_CIR(builder.JudgeAotAndFastCall(func, CircuitBuilder::JudgeMethodType::HAS_AOT),
                &slowCall, &slowPath);
            builder.Bind(&slowCall);
            {
                GateRef expectedArgc = builder.Int64Add(builder.GetExpectedNumOfArgs(method),
                    builder.Int64(NUM_MANDATORY_JSFUNC_ARGS));
                BRANCH_CIR(builder.Equal(expectedArgc, argc), &call1, &callBridge1);
                builder.Bind(&call1);
                {
                    builder.StartCallTimer(glue, gate, {glue, func, builder.True()}, true);
                    GateRef code = builder.GetCodeAddr(func);
                    auto depend = builder.GetDepend();
                    const CallSignature *cs = RuntimeStubCSigns::GetOptimizedCallSign();
                    result->WriteVariable(builder.Call(cs, glue, code, depend, args, gate, "callAOT"));
                    builder.EndCallTimer(glue, gate, {glue, func}, true);
                    builder.Jump(exit);
                }
                builder.Bind(&callBridge1);
                {
                    builder.StartCallTimer(glue, gate, {glue, func, builder.True()}, true);
                    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(OptimizedCallAndPushArgv));
                    GateRef target = builder.IntPtr(RTSTUB_ID(OptimizedCallAndPushArgv));
                    auto depend = builder.GetDepend();
                    result->WriteVariable(builder.Call(cs, glue, target, depend, args, gate, "callBridge"));
                    builder.EndCallTimer(glue, gate, {glue, func}, true);
                    builder.Jump(exit);
                }
            }
        }
    }
    builder.Bind(&slowPath);
    {
        if (isNew) {
            builder.StartCallTimer(glue, gate, {glue, func, builder.True()}, true);
            const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCallNew));
            GateRef target = builder.IntPtr(RTSTUB_ID(JSCallNew));
            auto depend = builder.GetDepend();
            result->WriteVariable(builder.Call(cs, glue, target, depend, args, gate, "slowNew"));
            builder.EndCallTimer(glue, gate, {glue, func}, true);
            builder.Jump(exit);
        } else {
            builder.StartCallTimer(glue, gate, {glue, func, builder.True()}, true);
            const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCall));
            GateRef target = builder.IntPtr(RTSTUB_ID(JSCall));
            auto depend = builder.GetDepend();
            result->WriteVariable(builder.Call(cs, glue, target, depend, args, gate, "jscall"));
            builder.EndCallTimer(glue, gate, {glue, func}, true);
            builder.Jump(exit);
        }
    }
}

GateRef CallCoStubBuilder::CallStubDispatch()
{
    std::vector<GateRef> args = {};
    std::vector<GateRef> argsFastCall = {};
    PrepareArgs(args, argsFastCall);

    auto env = GetEnvironment();
    auto &builder =  *env->GetBuilder();
    Label entry(env);
    Label exit(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), builder.Undefined());
    LowerFastCall(hirGate_, glue_, builder, func_, actualArgc_, args, argsFastCall,
                  &result, &exit, false);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef CallStubBuilder::JSCallDispatch()
{
    this->isForBaseline_ = false;
    auto env = GetEnvironment();
    Label entryPass(env);
    Label exit(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    this->result_ = &result;

    Label prepareForAsmBridgeEntry(env);
    Label finishPrepare(env);
    GateRef gcStateBitField = LoadPrimitive(VariableType::NATIVE_POINTER(), glue_,
        IntPtr(JSThread::GlueData::GetSharedGCStateBitFieldOffset(false)));
    GateRef readBarrierStateBit = Int64And(gcStateBitField, Int64(JSThread::READ_BARRIER_STATE_BITFIELD_MASK));
    BRANCH_LIKELY(Int64Equal(readBarrierStateBit, Int64(0)), &finishPrepare, &prepareForAsmBridgeEntry);
    Bind(&prepareForAsmBridgeEntry);
    {
        // func_ should be ToSpace Reference
        CallNGCRuntime(glue_, RTSTUB_ID(CopyCallTarget), { glue_, func_ });
        // every callmode except SUPER_CALL_SPREAD_WITH_ARGV and CALL_THIS_ARGV_WITH_RETURN has
        // argv on the frame which is GC root, so only need to copy argv here for these two types of callmode
        if (callArgs_.mode == JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV) {
            // argv should be ToSpace Reference
            CallNGCRuntime(glue_, RTSTUB_ID(CopyArgvArray),
                { glue_, callArgs_.superCallArgs.argv, callArgs_.superCallArgs.argc });
        } else if (callArgs_.mode == JSCallMode::CALL_THIS_ARGV_WITH_RETURN) {
            // argv should be ToSpace Reference
            CallNGCRuntime(glue_, RTSTUB_ID(CopyArgvArray),
                { glue_, callArgs_.callThisArgvWithReturnArgs.argv,
                    ZExtInt32ToInt64(callArgs_.callThisArgvWithReturnArgs.argc) });
        }
        Jump(&finishPrepare);
    }
    Bind(&finishPrepare);
    // 1. call initialize
    Label funcIsHeapObject(env);
    Label funcIsCallable(env);
    Label funcNotCallable(env);
    JSCallInit(&exit, &funcIsHeapObject, &funcIsCallable, &funcNotCallable);

    // 2. dispatch
    Label methodIsNative(env);
    Label methodNotNative(env);
    BRANCH(Int64NotEqual(Int64And(callField_, isNativeMask_), Int64(0)), &methodIsNative, &methodNotNative);

    // 3. call native
    Bind(&methodIsNative);
    {
        JSCallNative(&exit);
    }
    // 4. call nonNative
    Bind(&methodNotNative);
    {
        JSCallJSFunction(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void CallStubBuilder::JSCallInit(Label *exit, Label *funcIsHeapObject, Label *funcIsCallable, Label *funcNotCallable)
{
    if (!isForBaseline_) {
        // save pc
        SavePcIfNeeded(glue_);
    }
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    CallNGCRuntime(glue_, RTSTUB_ID(StartCallTimer, { glue_, func_, False()}));
#endif
    if (checkIsCallable_) {
        BRANCH_LIKELY(TaggedIsHeapObject(func_), funcIsHeapObject, funcNotCallable);
        Bind(funcIsHeapObject);
        GateRef hclass = LoadHClass(glue_, func_);
        bitfield_ = LoadPrimitive(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
        BRANCH_LIKELY(IsCallableFromBitField(bitfield_), funcIsCallable, funcNotCallable);
        Bind(funcNotCallable);
        {
            CallRuntime(glue_, RTSTUB_ID(ThrowNotCallableException), {func_});
            Jump(exit);
        }
        Bind(funcIsCallable);
    } else {
        GateRef hclass = LoadHClass(glue_, func_);
        bitfield_ = LoadPrimitive(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    }
    method_ = GetMethodFromJSFunctionOrProxy(glue_, func_);
    callField_ = GetCallFieldFromMethod(method_);
    isNativeMask_ = Int64(static_cast<uint64_t>(1) << MethodLiteral::IsNativeBit::START_BIT);
}

void CallStubBuilder::JSCallNative(Label *exit)
{
    HandleProfileNativeCall();
    newTarget_ = Undefined();
    thisValue_ = Undefined();
    numArgs_ = Int32Add(actualNumArgs_, Int32(NUM_MANDATORY_JSFUNC_ARGS));
    auto env = GetEnvironment();
    Label jsProxy(env);
    Label notJsProxy(env);
    BRANCH(IsJsProxy(glue_, func_), &jsProxy, &notJsProxy);
    Bind(&jsProxy);
    {
        JSCallNativeInner(exit, true);
    }
    Bind(&notJsProxy);
    {
        JSCallNativeInner(exit, false);
    }
}

void CallStubBuilder::JSCallNativeInner(Label *exit, bool isJsProxy)
{
    if (isJsProxy) {
        nativeCode_ = LoadPrimitive(VariableType::NATIVE_POINTER(), method_,
            IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    } else {
        nativeCode_ = LoadPrimitive(VariableType::NATIVE_POINTER(), func_, IntPtr(JSFunctionBase::CODE_ENTRY_OFFSET));
    }
    GateRef ret;
    int idxForNative = PrepareIdxForNative();
    std::vector<GateRef> argsForNative = PrepareArgsForNative();
    auto env = GetEnvironment();
    Label notFastBuiltins(env);
    switch (callArgs_.mode) {
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            if (!isJsProxy) {
                GateRef numArgsKeeper = numArgs_;
                CallFastBuiltin(&notFastBuiltins, exit, hir_);
                Bind(&notFastBuiltins);
                numArgs_ = numArgsKeeper;
            }
            [[fallthrough]];
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            ret = CallNGCRuntime(glue_, idxForNative, argsForNative, hir_);
            break;
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            ret = CallRuntime(glue_, idxForNative, argsForNative);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    result_->WriteVariable(ret);
    Jump(exit);
}

void CallStubBuilder::JSCallJSFunction(Label *exit, Label *noNeedCheckException)
{
    auto env = GetEnvironment();

    Label funcIsClassConstructor(env);
    Label funcNotClassConstructor(env);
    Label methodNotAot(env);

    if (!AssemblerModule::IsCallNew(callArgs_.mode)) {
        BRANCH(IsClassConstructorFromBitField(bitfield_), &funcIsClassConstructor, &funcNotClassConstructor);
        Bind(&funcIsClassConstructor);
        {
            CallRuntime(glue_, RTSTUB_ID(ThrowCallConstructorException), {});
            Jump(exit);
        }
        Bind(&funcNotClassConstructor);
    }
    HandleProfileCall();

#ifdef USE_READ_BARRIER
    CallNGCRuntime(glue_, RTSTUB_ID(CopyCallTarget), { glue_, func_ });
    if (callArgs_.mode == JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV) {
        CallNGCRuntime(glue_, RTSTUB_ID(CopyArgvArray),
            { glue_, callArgs_.superCallArgs.argv, callArgs_.superCallArgs.argc });
    }
#endif

    if (isForBaseline_ && env->IsBaselineBuiltin()) {
        sp_ = Argument(static_cast<size_t>(BaselineCallInputs::SP));
    }
    if (!isForBaseline_ && env->IsAsmInterp()) {
        sp_ = Argument(static_cast<size_t>(InterpreterHandlerInputs::SP));
    }
    Label methodisAot(env);
    Label funcHasBaselineCode(env);
    Label funcCheckBaselineCode(env);
    Label checkIsBaselineCompiling(env);
    Label methodIsFastCall(env);
    Label methodNotFastCall(env);
    Label checkAot(env);
    {
        newTarget_ = Undefined();
        thisValue_ = Undefined();
        realNumArgs_ = Int64Add(ZExtInt32ToInt64(actualNumArgs_), Int64(NUM_MANDATORY_JSFUNC_ARGS));
        BRANCH(IsJsProxy(glue_, func_), &methodNotAot, &checkAot);
        Bind(&checkAot);
        BRANCH(JudgeAotAndFastCall(func_, CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL), &methodIsFastCall,
            &methodNotFastCall);
        Bind(&methodIsFastCall);
        {
            JSFastAotCall(exit);
        }

        Bind(&methodNotFastCall);
        BRANCH(JudgeAotAndFastCall(func_, CircuitBuilder::JudgeMethodType::HAS_AOT), &methodisAot,
            &funcCheckBaselineCode);
        Bind(&methodisAot);
        {
            JSSlowAotCall(exit);
        }

        Bind(&funcCheckBaselineCode);
        GateRef baselineCodeOffset = IntPtr(JSFunction::BASELINECODE_OFFSET);
        // jump over the ReadBarrier for baselineCode since currently baseline-jit is disabled
        GateRef baselineCode = LoadPrimitive(VariableType::JS_POINTER(), func_, baselineCodeOffset);

        Branch(NotEqual(baselineCode, Undefined()), &checkIsBaselineCompiling, &methodNotAot);
        Bind(&checkIsBaselineCompiling);
        Branch(NotEqual(baselineCode, Hole()), &funcHasBaselineCode, &methodNotAot);

        Bind(&funcHasBaselineCode);
        {
            GateRef res = result_->Value();
            JSCallAsmInterpreter(true, exit, noNeedCheckException);
            (void) res;
        }

        Bind(&methodNotAot);
        {
            JSCallAsmInterpreter(false, exit, noNeedCheckException);
        }
    }
}

void CallStubBuilder::JSFastAotCall(Label *exit)
{
    auto env = GetEnvironment();
    Label fastCall(env);
    Label fastCallBridge(env);
    isFast_ = true;

    GateRef expectedNum = Int64And(Int64LSR(callField_, Int64(MethodLiteral::NumArgsBits::START_BIT)),
        Int64((1LU << MethodLiteral::NumArgsBits::SIZE) - 1));
    GateRef expectedArgc = Int64Add(expectedNum, Int64(NUM_MANDATORY_JSFUNC_ARGS));
    BRANCH(Int64Equal(expectedArgc, realNumArgs_), &fastCall, &fastCallBridge);
    GateRef code;
    Bind(&fastCall);
    {
        isBridge_ = false;
        code = GetAotCodeAddr(func_);
        CallBridge(code, expectedNum, exit);
    }
    Bind(&fastCallBridge);
    {
        isBridge_ = true;
        CallBridge(code, expectedNum, exit);
    }
}

void CallStubBuilder::JSSlowAotCall(Label *exit)
{
    auto env = GetEnvironment();
    Label slowCall(env);
    Label slowCallBridge(env);
    isFast_ = false;

    GateRef expectedNum = Int64And(Int64LSR(callField_, Int64(MethodLiteral::NumArgsBits::START_BIT)),
        Int64((1LU << MethodLiteral::NumArgsBits::SIZE) - 1));
    GateRef expectedArgc = Int64Add(expectedNum, Int64(NUM_MANDATORY_JSFUNC_ARGS));
    BRANCH(Int64Equal(expectedArgc, realNumArgs_), &slowCall, &slowCallBridge);
    GateRef code;
    Bind(&slowCall);
    {
        isBridge_ = false;
        code = GetAotCodeAddr(func_);
        CallBridge(code, expectedNum, exit);
    }
    Bind(&slowCallBridge);
    {
        isBridge_ = true;
        CallBridge(code, expectedNum, exit);
    }
}

GateRef CallStubBuilder::CallConstructorBridge(const int idxForAot, const std::vector<GateRef> &argsForAot)
{
    GateRef ret;
    switch (callArgs_.mode) {
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            ret = CallNGCRuntime(glue_, idxForAot, argsForAot);
            ret = ConstructorCheck(glue_, func_, ret, callArgs_.callConstructorArgs.thisObj);
            break;
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            ret = CallNGCRuntime(glue_, idxForAot, argsForAot, hir_);
            ret = ConstructorCheck(glue_, func_, ret, callArgs_.superCallArgs.thisObj);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return ret;
}

void CallStubBuilder::CallBridge(GateRef code, GateRef expectedNum, Label *exit)
{
    int idxForAot = PrepareIdxForAot();
    std::vector<GateRef> argsForAot = PrepareArgsForAot(expectedNum);
    GateRef ret;
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            if (IsFastAotCall()) {
                ret = FastCallOptimized(glue_, code, argsForAot, hir_);
            } else if (IsSlowAotCall()) {
                ret = CallOptimized(glue_, code, argsForAot, hir_);
            } else {
                ret = CallNGCRuntime(glue_, idxForAot, argsForAot, hir_);
            }
            break;
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            ret = CallNGCRuntime(glue_, idxForAot, argsForAot);
            break;
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            ret = CallConstructorBridge(idxForAot, argsForAot);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    result_->WriteVariable(ret);
    Jump(exit);
}

void CallStubBuilder::JSCallAsmInterpreter(bool hasBaselineCode, Label *exit,
    Label *noNeedCheckException)
{
    if (jumpSize_ != 0) {
        SaveJumpSizeIfNeeded(glue_, jumpSize_);
    }
    SaveHotnessCounterIfNeeded(glue_, sp_, hotnessCounter_, callArgs_.mode);

    int idxForAsmInterpreter = isForBaseline_ ?
        (hasBaselineCode ?
            PrepareIdxForAsmInterpreterForBaselineWithBaselineCode() :
            PrepareIdxForAsmInterpreterForBaselineWithoutBaselineCode()) :
        (hasBaselineCode ?
            PrepareIdxForAsmInterpreterWithBaselineCode() :
            PrepareIdxForAsmInterpreterWithoutBaselineCode());
    std::vector<GateRef> argsForAsmInterpreter = PrepareArgsForAsmInterpreter();

    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            *result_ = CallNGCRuntime(glue_, idxForAsmInterpreter, argsForAsmInterpreter);
            if (!isForBaseline_) {
                Return();
            }
            break;
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            if (isForBaseline_) {
                *result_ = CallNGCRuntime(glue_, idxForAsmInterpreter, argsForAsmInterpreter);
            } else {
                *result_ = CallNGCRuntime(glue_, idxForAsmInterpreter, argsForAsmInterpreter, hir_);
                Jump(exit);
            }
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    if (isForBaseline_) {
        if (noNeedCheckException != nullptr) {
            Jump(noNeedCheckException);
        } else {
            Jump(exit);
        }
    }
}

int CallStubBuilder::PrepareIdxForNative()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return RTSTUB_ID(PushCallArgsAndDispatchNative);
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return RTSTUB_ID(PushCallRangeAndDispatchNative);
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return RTSTUB_ID(PushCallNewAndDispatchNative);
        case JSCallMode::SUPER_CALL_WITH_ARGV:
            return RTSTUB_ID(SuperCall);
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return RTSTUB_ID(SuperCallSpread);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareArgsForNative()
{
    std::vector<GateRef> basicArgs = PrepareBasicArgsForNative();
    std::vector<GateRef> appendArgs = PrepareAppendArgsForNative();

    basicArgs.insert(basicArgs.end(), appendArgs.begin(), appendArgs.end());
    return basicArgs;
}

std::vector<GateRef> CallStubBuilder::PrepareBasicArgsForNative()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            return { nativeCode_, glue_, numArgs_, func_, newTarget_, thisValue_ };
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return { nativeCode_, glue_, numArgs_, func_, newTarget_ };
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            return { glue_, nativeCode_, func_, thisValue_ };
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return { glue_, nativeCode_, func_ };
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return {};
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareAppendArgsForNative()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG0:
            return {};
        case JSCallMode::CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG1:
            return {
                callArgs_.callArgs.arg0
            };
        case JSCallMode::CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG2:
            return {
                callArgs_.callArgs.arg0,
                callArgs_.callArgs.arg1
            };
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            return {
                callArgs_.callArgs.arg0,
                callArgs_.callArgs.arg1,
                callArgs_.callArgs.arg2
            };
        case JSCallMode::CALL_THIS_ARG0:
            return {
                callArgs_.callArgsWithThis.thisValue
            };
        case JSCallMode::CALL_THIS_ARG1:
            return {
                callArgs_.callArgsWithThis.thisValue,
                callArgs_.callArgsWithThis.arg0
            };
        case JSCallMode::CALL_THIS_ARG2:
            return {
                callArgs_.callArgsWithThis.thisValue,
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.arg1
            };
        case JSCallMode::CALL_THIS_ARG3:
            return {
                callArgs_.callArgsWithThis.thisValue,
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.arg1,
                callArgs_.callArgsWithThis.arg2
            };
        case JSCallMode::CALL_GETTER:
            return {
                callArgs_.callGetterArgs.receiver
            };
        case JSCallMode::CALL_SETTER:
            return {
                callArgs_.callSetterArgs.receiver,
                callArgs_.callSetterArgs.value
            };
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return {
                callArgs_.callThisArg2WithReturnArgs.thisValue,
                callArgs_.callThisArg2WithReturnArgs.arg0,
                callArgs_.callThisArg2WithReturnArgs.arg1
            };
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return {
                callArgs_.callThisArg3WithReturnArgs.argHandle,
                callArgs_.callThisArg3WithReturnArgs.value,
                callArgs_.callThisArg3WithReturnArgs.key,
                callArgs_.callThisArg3WithReturnArgs.thisValue
            };
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            return {
                callArgs_.callArgv.argc,
                callArgs_.callArgv.argv
            };
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
            return {
                callArgs_.callArgvWithThis.thisValue,
                callArgs_.callArgvWithThis.argc,
                callArgs_.callArgvWithThis.argv
            };
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return {
                callArgs_.callThisArgvWithReturnArgs.thisValue,
                callArgs_.callThisArgvWithReturnArgs.argc,
                callArgs_.callThisArgvWithReturnArgs.argv
            };
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return {
                callArgs_.callConstructorArgs.thisObj,
                callArgs_.callConstructorArgs.argc,
                callArgs_.callConstructorArgs.argv
            };
        case JSCallMode::SUPER_CALL_WITH_ARGV:
            return {
                callArgs_.superCallArgs.thisFunc,
                callArgs_.superCallArgs.array,
                IntToTaggedInt(callArgs_.superCallArgs.argc)
            };
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return {
                callArgs_.superCallArgs.thisFunc,
                callArgs_.superCallArgs.array
            };
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

int CallStubBuilder::PrepareIdxForAot()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            if (isFast_) {
                return RTSTUB_ID(OptimizedFastCallAndPushArgv);
            } else {
                return RTSTUB_ID(OptimizedCallAndPushArgv);
            }
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            if (IsFastAotCall()) {
                return RTSTUB_ID(JSFastCallWithArgV);
            } else if (IsFastAotCallWithBridge()) {
                return RTSTUB_ID(JSFastCallWithArgVAndPushArgv);
            } else if (IsSlowAotCall()) {
                return RTSTUB_ID(JSCallWithArgV);
            } else {
                return RTSTUB_ID(JSCallWithArgVAndPushArgv);
            }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareArgsForAot(GateRef expectedNum)
{
    std::vector<GateRef> basicArgs = PrepareBasicArgsForAot();

    std::vector<GateRef> appendArgs = PrepareAppendArgsForAotStep1();
    basicArgs.insert(basicArgs.end(), appendArgs.begin(), appendArgs.end());

    appendArgs = PrepareAppendArgsForAotStep2();
    basicArgs.insert(basicArgs.end(), appendArgs.begin(), appendArgs.end());

    appendArgs = PrepareAppendArgsForAotStep3(expectedNum);
    basicArgs.insert(basicArgs.end(), appendArgs.begin(), appendArgs.end());

    return basicArgs;
}

std::vector<GateRef> CallStubBuilder::PrepareBasicArgsForAot()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            if (IsFastAotCall()) {
                return { glue_, func_, thisValue_ };
            } else {
                return { glue_, realNumArgs_, IntPtr(0), func_, newTarget_, thisValue_ };
            }
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            if (IsFastAotCall()) {
                return { glue_, func_ };
            } else {
                return { glue_, realNumArgs_, IntPtr(0), func_, newTarget_ };
            }
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            if (isFast_) {
                return { glue_, func_, thisValue_, ZExtInt32ToInt64(actualNumArgs_) };
            } else {
                return { glue_, ZExtInt32ToInt64(actualNumArgs_), func_, newTarget_, thisValue_ };
            }
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            if (!isFast_) {
                return { glue_, ZExtInt32ToInt64(actualNumArgs_), func_, func_ };
            }
            [[fallthrough]];
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            if (isFast_) {
                return { glue_, func_ };
            } else {
                return { glue_, ZExtInt32ToInt64(actualNumArgs_), func_, newTarget_ };
            }
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            if (isFast_) {
                return { glue_, func_ };
            } else {
                return { glue_, ZExtInt32ToInt64(actualNumArgs_), func_};
            }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareAppendArgsForAotStep1()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG0:
            return {};
        case JSCallMode::CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG1:
            return { callArgs_.callArgs.arg0 };
        case JSCallMode::CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG2:
            return {
                callArgs_.callArgs.arg0,
                callArgs_.callArgs.arg1
            };
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            return {
                callArgs_.callArgs.arg0,
                callArgs_.callArgs.arg1,
                callArgs_.callArgs.arg2
            };
        case JSCallMode::CALL_THIS_ARG0:
            return { callArgs_.callArgsWithThis.thisValue };
        case JSCallMode::CALL_THIS_ARG1:
            return {
                callArgs_.callArgsWithThis.thisValue,
                callArgs_.callArgsWithThis.arg0
            };
        case JSCallMode::CALL_THIS_ARG2:
            return {
                callArgs_.callArgsWithThis.thisValue,
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.arg1
            };
        case JSCallMode::CALL_THIS_ARG3:
            return {
                callArgs_.callArgsWithThis.thisValue,
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.arg1,
                callArgs_.callArgsWithThis.arg2
            };
        case JSCallMode::CALL_GETTER:
            return { callArgs_.callGetterArgs.receiver };
        case JSCallMode::CALL_SETTER:
            return {
                callArgs_.callSetterArgs.receiver,
                callArgs_.callSetterArgs.value
            };
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return {
                callArgs_.callThisArg2WithReturnArgs.thisValue,
                callArgs_.callThisArg2WithReturnArgs.arg0,
                callArgs_.callThisArg2WithReturnArgs.arg1
            };
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return {
                callArgs_.callThisArg3WithReturnArgs.argHandle,
                callArgs_.callThisArg3WithReturnArgs.value,
                callArgs_.callThisArg3WithReturnArgs.key,
                callArgs_.callThisArg3WithReturnArgs.thisValue
            };
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            return { callArgs_.callArgv.argv };
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
            return { callArgs_.callArgvWithThis.thisValue };
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return { callArgs_.callConstructorArgs.thisObj };
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return { callArgs_.callThisArgvWithReturnArgs.thisValue };
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return {};
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareAppendArgsForAotStep2()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            if (isFast_) {
                return { ZExtInt32ToInt64(actualNumArgs_) };
            }
            [[fallthrough]];
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return {};
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            if (isFast_) {
                return {
                    callArgs_.superCallArgs.thisObj,
                    callArgs_.superCallArgs.argc,
                    callArgs_.superCallArgs.argv
                };
            } else {
                return {
                    callArgs_.superCallArgs.newTarget,
                    callArgs_.superCallArgs.thisObj,
                    callArgs_.superCallArgs.argv
                };
            }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareAppendArgsForAotStep3(GateRef expectedNum)
{
    std::vector<GateRef> retArgs {};
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return {};
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
            retArgs.push_back(callArgs_.callArgvWithThis.argv);
            break;
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            retArgs.push_back(callArgs_.callConstructorArgs.argv);
            break;
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            retArgs.push_back(callArgs_.callThisArgvWithReturnArgs.argv);
            break;
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    if (IsFastAotCallWithBridge()) {
        retArgs.push_back(expectedNum);
    }
    return retArgs;
}

int CallStubBuilder::PrepareIdxForAsmInterpreterForBaselineWithBaselineCode()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG0:
            return RTSTUB_ID(CallArg0AndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG1:
            return RTSTUB_ID(CallArg1AndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG2:
            return RTSTUB_ID(CallArgs2AndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            return RTSTUB_ID(CallArgs3AndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_THIS_ARG0:
            return RTSTUB_ID(CallThisArg0AndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_THIS_ARG1:
            return RTSTUB_ID(CallThisArg1AndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_THIS_ARG2:
            return RTSTUB_ID(CallThisArgs2AndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_THIS_ARG3:
            return RTSTUB_ID(CallThisArgs3AndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            return RTSTUB_ID(CallRangeAndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
            return RTSTUB_ID(CallThisRangeAndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return RTSTUB_ID(CallNewAndCheckToBaselineFromBaseline);
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return RTSTUB_ID(SuperCallAndCheckToBaselineFromBaseline);
        case JSCallMode::CALL_GETTER:
            return RTSTUB_ID(CallGetterToBaseline);
        case JSCallMode::CALL_SETTER:
            return RTSTUB_ID(CallSetterToBaseline);
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return RTSTUB_ID(CallContainersArgs2ToBaseline);
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return RTSTUB_ID(CallContainersArgs3ToBaseline);
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return RTSTUB_ID(CallReturnWithArgvToBaseline);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

int CallStubBuilder::PrepareIdxForAsmInterpreterForBaselineWithoutBaselineCode()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG0:
            return RTSTUB_ID(CallArg0AndDispatchFromBaseline);
        case JSCallMode::CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG1:
            return RTSTUB_ID(CallArg1AndDispatchFromBaseline);
        case JSCallMode::CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG2:
            return RTSTUB_ID(CallArgs2AndDispatchFromBaseline);
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            return RTSTUB_ID(CallArgs3AndDispatchFromBaseline);
        case JSCallMode::CALL_THIS_ARG0:
            return RTSTUB_ID(CallThisArg0AndDispatchFromBaseline);
        case JSCallMode::CALL_THIS_ARG1:
            return RTSTUB_ID(CallThisArg1AndDispatchFromBaseline);
        case JSCallMode::CALL_THIS_ARG2:
            return RTSTUB_ID(CallThisArgs2AndDispatchFromBaseline);
        case JSCallMode::CALL_THIS_ARG3:
            return RTSTUB_ID(CallThisArgs3AndDispatchFromBaseline);
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            return RTSTUB_ID(CallRangeAndDispatchFromBaseline);
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
            return RTSTUB_ID(CallThisRangeAndDispatchFromBaseline);
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return RTSTUB_ID(CallNewAndDispatchFromBaseline);
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return RTSTUB_ID(SuperCallAndDispatchFromBaseline);
        case JSCallMode::CALL_GETTER:
            return RTSTUB_ID(CallGetter);
        case JSCallMode::CALL_SETTER:
            return RTSTUB_ID(CallSetter);
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return RTSTUB_ID(CallContainersArgs2);
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return RTSTUB_ID(CallContainersArgs3);
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return RTSTUB_ID(CallReturnWithArgv);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

int CallStubBuilder::PrepareIdxForAsmInterpreterWithBaselineCode()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG0:
            return RTSTUB_ID(CallArg0AndCheckToBaseline);
        case JSCallMode::CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG1:
            return RTSTUB_ID(CallArg1AndCheckToBaseline);
        case JSCallMode::CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG2:
            return RTSTUB_ID(CallArgs2AndCheckToBaseline);
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            return RTSTUB_ID(CallArgs3AndCheckToBaseline);
        case JSCallMode::CALL_THIS_ARG0:
            return RTSTUB_ID(CallThisArg0AndCheckToBaseline);
        case JSCallMode::CALL_THIS_ARG1:
            return RTSTUB_ID(CallThisArg1AndCheckToBaseline);
        case JSCallMode::CALL_THIS_ARG2:
            return RTSTUB_ID(CallThisArgs2AndCheckToBaseline);
        case JSCallMode::CALL_THIS_ARG3:
            return RTSTUB_ID(CallThisArgs3AndCheckToBaseline);
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            return RTSTUB_ID(CallRangeAndCheckToBaseline);
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
            return RTSTUB_ID(CallThisRangeAndCheckToBaseline);
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return RTSTUB_ID(CallNewAndCheckToBaseline);
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return RTSTUB_ID(SuperCallAndCheckToBaseline);
        case JSCallMode::CALL_GETTER:
            return RTSTUB_ID(CallGetterToBaseline);
        case JSCallMode::CALL_SETTER:
            return RTSTUB_ID(CallSetterToBaseline);
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return RTSTUB_ID(CallContainersArgs2ToBaseline);
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return RTSTUB_ID(CallContainersArgs3ToBaseline);
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return RTSTUB_ID(CallReturnWithArgvToBaseline);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

int CallStubBuilder::PrepareIdxForAsmInterpreterWithoutBaselineCode()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG0:
            return RTSTUB_ID(PushCallArg0AndDispatch);
        case JSCallMode::CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG1:
            return RTSTUB_ID(PushCallArg1AndDispatch);
        case JSCallMode::CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG2:
            return RTSTUB_ID(PushCallArgs2AndDispatch);
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            return RTSTUB_ID(PushCallArgs3AndDispatch);
        case JSCallMode::CALL_THIS_ARG0:
            return RTSTUB_ID(PushCallThisArg0AndDispatch);
        case JSCallMode::CALL_THIS_ARG1:
            return RTSTUB_ID(PushCallThisArg1AndDispatch);
        case JSCallMode::CALL_THIS_ARG2:
            return RTSTUB_ID(PushCallThisArgs2AndDispatch);
        case JSCallMode::CALL_THIS_ARG3:
            return RTSTUB_ID(PushCallThisArgs3AndDispatch);
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            return RTSTUB_ID(PushCallRangeAndDispatch);
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
            return RTSTUB_ID(PushCallThisRangeAndDispatch);
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return RTSTUB_ID(PushCallNewAndDispatch);
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return RTSTUB_ID(PushSuperCallAndDispatch);
        case JSCallMode::CALL_GETTER:
            return RTSTUB_ID(CallGetter);
        case JSCallMode::CALL_SETTER:
            return RTSTUB_ID(CallSetter);
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return RTSTUB_ID(CallContainersArgs2);
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return RTSTUB_ID(CallContainersArgs3);
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return RTSTUB_ID(CallReturnWithArgv);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareArgsForAsmInterpreter()
{
    std::vector<GateRef> basicArgs = PrepareBasicArgsForAsmInterpreter();
    std::vector<GateRef> appendArgs = PrepareAppendArgsForAsmInterpreter();

    basicArgs.insert(basicArgs.end(), appendArgs.begin(), appendArgs.end());
    return basicArgs;
}

std::vector<GateRef> CallStubBuilder::PrepareBasicArgsForAsmInterpreter()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            if (isForBaseline_) {
                return { glue_, baselineBuiltinFp_, func_, method_, callField_ };
            } else {
                return { glue_, sp_, func_, method_, callField_ };
            }
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return { glue_, func_, method_, callField_ };
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareAppendArgsForAsmInterpreter()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG0:
            return {};
        case JSCallMode::CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG1:
            return { callArgs_.callArgs.arg0 };
        case JSCallMode::CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG2:
            return {
                callArgs_.callArgs.arg0,
                callArgs_.callArgs.arg1
            };
        case JSCallMode::CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_ARG3:
            return {
                callArgs_.callArgs.arg0,
                callArgs_.callArgs.arg1,
                callArgs_.callArgs.arg2
            };
        case JSCallMode::CALL_THIS_ARG0:
            return { callArgs_.callArgsWithThis.thisValue };
        case JSCallMode::CALL_THIS_ARG1:
            return {
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.thisValue
            };
        case JSCallMode::CALL_THIS_ARG2:
            return {
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.arg1,
                callArgs_.callArgsWithThis.thisValue
            };
        case JSCallMode::CALL_THIS_ARG3:
            return {
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.arg1,
                callArgs_.callArgsWithThis.arg2,
                callArgs_.callArgsWithThis.thisValue
            };
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
            return {
                callArgs_.callArgv.argc,
                callArgs_.callArgv.argv
            };
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
            return {
                callArgs_.callArgvWithThis.argc,
                callArgs_.callArgvWithThis.argv,
                callArgs_.callArgvWithThis.thisValue
            };
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return {
                callArgs_.callConstructorArgs.argc,
                callArgs_.callConstructorArgs.argv,
                callArgs_.callConstructorArgs.thisObj
            };
        case JSCallMode::CALL_GETTER:
            return { callArgs_.callGetterArgs.receiver };
        case JSCallMode::CALL_SETTER:
            return {
                callArgs_.callSetterArgs.value,
                callArgs_.callSetterArgs.receiver
            };
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return {
                callArgs_.callThisArg2WithReturnArgs.arg0,
                callArgs_.callThisArg2WithReturnArgs.arg1,
                callArgs_.callThisArg2WithReturnArgs.thisValue
            };
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return {
                callArgs_.callThisArg3WithReturnArgs.value,
                callArgs_.callThisArg3WithReturnArgs.key,
                callArgs_.callThisArg3WithReturnArgs.thisValue,
                callArgs_.callThisArg3WithReturnArgs.argHandle
            };
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return {
                callArgs_.callThisArgvWithReturnArgs.argc,
                callArgs_.callThisArgvWithReturnArgs.argv,
                callArgs_.callThisArgvWithReturnArgs.thisValue
            };
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return {
                callArgs_.superCallArgs.argc,
                callArgs_.superCallArgs.argv,
                callArgs_.superCallArgs.thisObj,
                callArgs_.superCallArgs.newTarget
            };
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void CallStubBuilder::CallFastBuiltin(Label* notFastBuiltins, Label *exit, GateRef hir)
{
    auto env = GetEnvironment();
    Label isFastBuiltins(env);
    Label supportCall(env);
    numArgs_ = ZExtInt32ToPtr(actualNumArgs_);
    GateRef isFastBuiltinsMask = Int64(static_cast<uint64_t>(1) << MethodLiteral::IsFastBuiltinBit::START_BIT);
    BRANCH(Int64NotEqual(Int64And(callField_, isFastBuiltinsMask), Int64(0)), &isFastBuiltins, notFastBuiltins);
    Bind(&isFastBuiltins);
    GateRef builtinId = GetBuiltinId(method_);
    if (IsCallModeSupportCallBuiltin()) {
        BRANCH(Int32GreaterThanOrEqual(builtinId, Int32(BUILTINS_STUB_ID(BUILTINS_CONSTRUCTOR_STUB_FIRST))),
            notFastBuiltins, &supportCall);
        Bind(&supportCall);
    }
    {
        GateRef ret;
        switch (callArgs_.mode) {
            case JSCallMode::CALL_THIS_ARG0:
            case JSCallMode::CALL_THIS_ARG1:
            case JSCallMode::CALL_THIS_ARG2:
            case JSCallMode::CALL_THIS_ARG3:
            case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
                ret = DispatchBuiltins(glue_, builtinId, PrepareArgsForFastBuiltin(), hir);
                break;
            case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                ret = DispatchBuiltinsWithArgv(glue_, builtinId, PrepareArgsForFastBuiltin());
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        result_->WriteVariable(ret);
        Jump(exit);
    }
}

std::vector<GateRef> CallStubBuilder::PrepareBasicArgsForFastBuiltin()
{
    return { glue_, nativeCode_, func_ };
}

std::vector<GateRef> CallStubBuilder::PrepareAppendArgsForFastBuiltin()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_THIS_ARG0:
            return { Undefined(),
                callArgs_.callArgsWithThis.thisValue, numArgs_, Undefined(), Undefined(), Undefined()
            };
        case JSCallMode::CALL_THIS_ARG1:
            return { Undefined(),
                callArgs_.callArgsWithThis.thisValue, numArgs_,
                callArgs_.callArgsWithThis.arg0, Undefined(), Undefined()
            };
        case JSCallMode::CALL_THIS_ARG2:
            return { Undefined(),
                callArgs_.callArgsWithThis.thisValue, numArgs_,
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.arg1, Undefined()
            };
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return { Undefined(),
                callArgs_.callThisArg2WithReturnArgs.thisValue, numArgs_,
                callArgs_.callThisArg2WithReturnArgs.arg0,
                callArgs_.callThisArg2WithReturnArgs.arg1, Undefined()
            };
        case JSCallMode::CALL_THIS_ARG3:
            return { Undefined(),
                callArgs_.callArgsWithThis.thisValue, numArgs_,
                callArgs_.callArgsWithThis.arg0,
                callArgs_.callArgsWithThis.arg1,
                callArgs_.callArgsWithThis.arg2
            };
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
            return { func_, thisValue_, numArgs_,
                callArgs_.callConstructorArgs.argv
            };
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

std::vector<GateRef> CallStubBuilder::PrepareArgsForFastBuiltin()
{
    std::vector<GateRef> basicArgs = PrepareBasicArgsForFastBuiltin();
    std::vector<GateRef> appendArgs = PrepareAppendArgsForFastBuiltin();

    basicArgs.insert(basicArgs.end(), appendArgs.begin(), appendArgs.end());
    return basicArgs;
}

bool CallStubBuilder::IsCallModeSupportPGO() const
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
            return true;
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_ENTRY:
        case JSCallMode::CALL_FROM_AOT:
        case JSCallMode::CALL_GENERATOR:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return false;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

bool CallStubBuilder::IsCallModeSupportCallBuiltin() const
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
            return true;
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            return false;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

bool CallStubBuilder::CheckResultValueChangedWithReturn(GateRef prevResRef) const
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return result_->Value() == prevResRef;
        default:
            return result_->Value() != prevResRef;
    }
}

bool CallStubBuilder::IsSlowAotCall() const
{ return !isFast_ && !isBridge_; }

bool CallStubBuilder::IsFastAotCall() const
{ return isFast_ && !isBridge_; }

bool CallStubBuilder::IsSlowAotCallWithBridge() const
{ return !isFast_ && isBridge_; }

bool CallStubBuilder::IsFastAotCallWithBridge() const
{ return isFast_ && isBridge_; }

void CallStubBuilder::HandleProfileCall()
{
    if (!callback_.IsEmpty()) {
        if (!IsCallModeSupportPGO()) {
            return;
        }
        if (IsCallModeGetterSetter()) {
            callback_.ProfileGetterSetterCall(func_);
            return;
        }
        callback_.ProfileCall(func_);
    }
}

void CallStubBuilder::HandleProfileNativeCall()
{
    if (!callback_.IsEmpty()) {
        if (!IsCallModeSupportPGO()) {
            return;
        }
        if (!IsCallModeGetterSetter()) {
            callback_.ProfileNativeCall(func_);
        }
    }
}

bool CallStubBuilder::IsCallModeGetterSetter()
{
    switch (callArgs_.mode) {
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
            return true;
        default:
            return false;
    }
}

} // panda::ecmascript::kungfu
