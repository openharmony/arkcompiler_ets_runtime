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

#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {

void CallStubBuilder::JSCallDispatchForBaseline(Label *exit, Label *noNeedCheckException)
{
    this->isForBaseline_ = true;
    auto env = GetEnvironment();
    baselineBuiltinFp_ = CallNGCRuntime(glue_, RTSTUB_ID(GetBaselineBuiltinFp), {glue_});

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

GateRef CallStubBuilder::JSCallDispatch()
{
    this->isForBaseline_ = false;
    auto env = GetEnvironment();
    Label entryPass(env);
    Label exit(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    this->result_ = &result;

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
        BRANCH(TaggedIsHeapObject(func_), funcIsHeapObject, funcNotCallable);
        Bind(funcIsHeapObject);
        GateRef hclass = LoadHClass(func_);
        bitfield_ = Load(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
        BRANCH(IsCallableFromBitField(bitfield_), funcIsCallable, funcNotCallable);
        Bind(funcNotCallable);
        {
            CallRuntime(glue_, RTSTUB_ID(ThrowNotCallableException), {});
            Jump(exit);
        }
        Bind(funcIsCallable);
    } else {
        GateRef hclass = LoadHClass(func_);
        bitfield_ = Load(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    }
    method_ = GetMethodFromJSFunctionOrProxy(func_);
    callField_ = GetCallFieldFromMethod(method_);
    isNativeMask_ = Int64(static_cast<uint64_t>(1) << MethodLiteral::IsNativeBit::START_BIT);
}

void CallStubBuilder::JSCallNative(Label *exit)
{
    HandleProfileNativeCall();
    GateRef ret;
    nativeCode_ = Load(VariableType::NATIVE_POINTER(), method_,
        IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    newTarget_ = Undefined();
    thisValue_ = Undefined();
    numArgs_ = Int32Add(actualNumArgs_, Int32(NUM_MANDATORY_JSFUNC_ARGS));
    GateRef numArgsKeeper;

    int idxForNative = PrepareIdxForNative();
    std::vector<GateRef> argsForNative = PrepareArgsForNative();
    auto env = GetEnvironment();
    Label notFastBuiltins(env);
    switch (callArgs_.mode) {
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            numArgsKeeper = numArgs_;
            CallFastBuiltin(&notFastBuiltins, exit);
            Bind(&notFastBuiltins);
            numArgs_ = numArgsKeeper;
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
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            ret = CallNGCRuntime(glue_, idxForNative, argsForNative);
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
    // Worker/Taskpool disable aot optimization
    Label judgeAotAndFastCall(env);
    Label checkAot(env);
    {
        newTarget_ = Undefined();
        thisValue_ = Undefined();
        realNumArgs_ = Int64Add(ZExtInt32ToInt64(actualNumArgs_), Int64(NUM_MANDATORY_JSFUNC_ARGS));
        BRANCH(IsJsProxy(func_), &methodNotAot, &checkAot);
        Bind(&checkAot);
        BRANCH(IsWorker(glue_), &funcCheckBaselineCode, &judgeAotAndFastCall);
        Bind(&judgeAotAndFastCall);
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
        GateRef baselineCode = Load(VariableType::JS_POINTER(), func_, baselineCodeOffset);

        Branch(NotEqual(baselineCode, Undefined()), &checkIsBaselineCompiling, &methodNotAot);
        Bind(&checkIsBaselineCompiling);
        Branch(NotEqual(baselineCode, Hole()), &funcHasBaselineCode, &methodNotAot);

        Bind(&funcHasBaselineCode);
        {
            GateRef res = result_->Value();
            JSCallAsmInterpreter(true, &methodNotAot, exit, noNeedCheckException);
            if (!isForBaseline_) {
                ASSERT(CheckResultValueChangedWithReturn(res));
            }
            (void) res;
        }

        Bind(&methodNotAot);
        {
            JSCallAsmInterpreter(false, &methodNotAot, exit, noNeedCheckException);
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
                ret = FastCallOptimized(glue_, code, argsForAot);
            } else if (IsSlowAotCall()) {
                ret = CallOptimized(glue_, code, argsForAot);
            } else {
                ret = CallNGCRuntime(glue_, idxForAot, argsForAot);
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
            ret = CallNGCRuntime(glue_, idxForAot, argsForAot);
            ret = ConstructorCheck(glue_, func_, ret, callArgs_.callConstructorArgs.thisObj);
            break;
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            ret = CallRuntime(glue_, idxForAot, argsForAot);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    result_->WriteVariable(ret);
    Jump(exit);
}

void CallStubBuilder::JSCallAsmInterpreter(bool hasBaselineCode, Label *methodNotAot, Label *exit,
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
            } else if (hasBaselineCode) {
                Jump(methodNotAot);
            } else {
                *result_ = CallNGCRuntime(glue_, idxForAsmInterpreter, argsForAsmInterpreter);
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
            if (IsFastAotCall()) {
                return RTSTUB_ID(JSFastCallWithArgV);
            } else if (IsFastAotCallWithBridge()) {
                return RTSTUB_ID(JSFastCallWithArgVAndPushArgv);
            } else if (IsSlowAotCall() && isForBaseline_) {
                return RTSTUB_ID(JSCallWithArgV);
            } else {
                return RTSTUB_ID(JSCallWithArgVAndPushArgv);
            }
        case JSCallMode::SUPER_CALL_WITH_ARGV:
            return RTSTUB_ID(SuperCall);
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return RTSTUB_ID(SuperCallSpread);
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
    
    appendArgs = PrepareAppendArgsForAotStep2(expectedNum);
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
            return {};
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

std::vector<GateRef> CallStubBuilder::PrepareAppendArgsForAotStep2(GateRef expectedNum)
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
            if (IsFastAotCallWithBridge()) {
                return { expectedNum };
            }
            [[fallthrough]];
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
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return {};
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
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
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

void CallStubBuilder::CallFastBuiltin(Label* notFastBuiltins, Label *exit)
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
        BRANCH(Int32GreaterThanOrEqual(builtinId, Int32(kungfu::BuiltinsStubCSigns::BUILTINS_CONSTRUCTOR_STUB_FIRST)),
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
                ret = DispatchBuiltins(glue_, builtinId, PrepareArgsForFastBuiltin());
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