/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/trampoline/aarch64/common_call.h"

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_function.h"
#include "ecmascript/method.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/message_string.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
#define __ assembler->

// * uint64_t CallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, uintptr_t arg0, ...)
// * webkit_jscc calling convention call runtime_id's runtime function(c-abi)
// * Arguments:
//         %x0 - glue
//
// * Optimized-leaved-frame layout as the following:
//         +--------------------------+
//         |       argv[N-1]          |
//         |--------------------------|
//         |       . . . . .          |
//         |--------------------------|
//         |       argv[0]            |
//         +--------------------------+-------------
//         |       argc               |            ^
//         |--------------------------|            |
//         |       RuntimeId          |            |
//  sp --> |--------------------------|   OptimizedLeaveFrame
//         |       ret-addr           |            |
//         |--------------------------|            |
//         |       prevFp             |            |
//         |--------------------------|            |
//         |       frameType          |            v
//         +--------------------------+-------------

void OptimizedCall::CallRuntime(ExtendedAssembler *assembler)
{
    Register glue(X0);
    Register fp(FP);
    Register tmp(X19);
    Register sp(SP);
    Register argC(X1);
    Register argV(X2);

    __ BindAssemblerStub(RTSTUB_ID(CallRuntime));
    __ PushFpAndLr();

    Register frameType(X2);
    // construct Leave Frame and callee save
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME)));
    // 2 : 2 means pairs
    __ Stp(tmp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(2 * FRAME_SLOT_SIZE));  // 16: skip frame type and tmp
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

    // load runtime trampoline address
    Register rtfunc(X19);
    __ Ldr(tmp, MemoryOperand(fp, GetStackArgOffSetToFp(0)));  // 0: the first arg id
    // 3 : 3 means 2 << 3 = 8
    __ Add(tmp, glue, Operand(tmp, LSL, 3));
    __ Ldr(rtfunc, MemoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Ldr(argC, MemoryOperand(fp, GetStackArgOffSetToFp(1)));  // 1: the second arg id
    __ Add(argV, fp, Immediate(GetStackArgOffSetToFp(2)));  // 2: the third arg id
    __ Blr(rtfunc);

    // callee restore
    // 0 : 0 restore size
    __ Ldr(tmp, MemoryOperand(sp, 0));

    // descontruct frame
    // 2 ：2 means stack frame slot size
    __ Add(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    __ RestoreFpAndLr();
    __ Ret();
}

void OptimizedCall::IncreaseStackForArguments(ExtendedAssembler *assembler, Register argc, Register currentSp)
{
    Register sp(SP);
    __ Mov(currentSp, sp);
    // add extra aguments, env and numArgs
    __ Add(argc, argc, Immediate(static_cast<int64_t>(CommonArgIdx::ACTUAL_ARGC)));
    __ Sub(currentSp, currentSp, Operand(argc, UXTW, FRAME_SLOT_SIZE_LOG2));
    Label aligned;
    __ Tst(currentSp, LogicalImmediate::Create(0xf, RegXSize));  // 0xf: 0x1111
    __ B(Condition::EQ, &aligned);
    __ Sub(currentSp, currentSp, Immediate(FRAME_SLOT_SIZE));
    __ Bind(&aligned);
    __ Mov(sp, currentSp);
    __ Add(currentSp, currentSp, Operand(argc, UXTW, FRAME_SLOT_SIZE_LOG2));
}

// * uint64_t JSFunctionEntry(uintptr_t glue, uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t prevFp,
//                            size_t callType)
// * Arguments:
//        %x0 - glue
//        %x1 - actualNumArgs
//        %x2 - argV
//        %x3 - prevFp
//        %x4 - callType
//
// * The JSFunctionEntry Frame's structure is illustrated as the following:
//          +--------------------------+
//          |      . . . . . .         |
//  sp ---> +--------------------------+ -----------------
//          |        prevFP            |                 ^
//          |--------------------------|                 |
//          |       frameType          |      JSFunctionEntryFrame
//          |--------------------------|                 |
//          |    preLeaveFrameFp       |                 v
//          +--------------------------+ -----------------

void OptimizedCall::JSFunctionEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSFunctionEntry));
    Register glueReg(X0);
    Register argV(X2);
    Register prevFpReg(X3);
    Register flag(X4);
    Register sp(SP);
    Register tmpArgV(X7);
    Label lJSCallNewWithArgV;
    Label lPopFrame;

    PushJSFunctionEntryFrame (assembler, prevFpReg);
    __ Mov(Register(X6), flag);
    __ Mov(tmpArgV, argV);
    __ Mov(Register(X20), glueReg);
    __ Ldr(Register(X2), MemoryOperand(tmpArgV, 0));
    __ Ldr(Register(X3), MemoryOperand(tmpArgV, FRAME_SLOT_SIZE));
    __ Ldr(Register(X4), MemoryOperand(tmpArgV, DOUBLE_SLOT_SIZE));
    __ Add(tmpArgV, tmpArgV, Immediate(TRIPLE_SLOT_SIZE));
    __ Mov(Register(X5), tmpArgV);
    __ Cmp(Register(X6), Immediate(1));
    __ B(Condition::EQ, &lJSCallNewWithArgV);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgV), false);
    __ B(&lPopFrame);

    __ Bind(&lJSCallNewWithArgV);
    {
        __ CallAssemblerStub(RTSTUB_ID(JSCallNewWithArgV), false);
    }

    __ Bind(&lPopFrame);
    __ Mov(Register(X2), Register(X20));
    PopJSFunctionEntryFrame(assembler, Register(X2));
    __ Ret();
}

// * uint64_t OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs, uint32_t actualNumArgs,
//                                   uintptr_t codeAddr, uintptr_t argv, uintptr_t lexEnv)
// * Arguments wil CC calling convention:
//         %x0 - glue
//         %x1 - codeAddr
//         %x2 - actualNumArgs
//         %x3 - expectedNumArgs
//         %x4  - argv
//         %x5  - lexEnv
//
// * The OptimizedJSFunctionArgsConfig Frame's structure is illustrated as the following:
//          +--------------------------+
//          |         arg[N-1]         |
//          +--------------------------+
//          |         . . . .          |
//          +--------------------------+
//          |         arg[0]           |
//          +--------------------------+
//          |         argC             |
//  sp ---> +--------------------------+ -----------------
//          |                          |                 ^
//          |        prevFP            |                 |
//          |--------------------------|    OptimizedJSFunctionArgsConfigFrame
//          |       frameType          |                 |
//          |                          |                 V
//          +--------------------------+ -----------------

void OptimizedCall::OptimizedCallOptimized(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedCallOptimized));
    Register glue(X0);
    Register expectedNumArgs(X1);
    Register actualNumArgs(X2);
    Register codeAddr(X3);
    Register argV(X4);
    Register env(X5);
    Register currentSp(X6);
    Register sp(SP);
    Label copyArguments;
    Label invokeCompiledJSFunction;

    // construct frame
    PushOptimizedArgsConfigFrame(assembler);
    Register argC(X7);
    __ Cmp(expectedNumArgs, actualNumArgs);
    __ CMov(argC, expectedNumArgs, actualNumArgs, Condition::HI);
    IncreaseStackForArguments(assembler, argC, currentSp);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register tmp = __ TempRegister1();
        Register undefinedValue = __ TempRegister2();
        __ Subs(tmp, expectedNumArgs, actualNumArgs);
        __ B(Condition::LS, &copyArguments);
        PushUndefinedWithArgc(assembler, glue, tmp, undefinedValue, currentSp, nullptr, nullptr);
    }
    __ Bind(&copyArguments);
    __ Cbz(actualNumArgs, &invokeCompiledJSFunction);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register argc = __ TempRegister1();
        Register argValue = __ TempRegister2();
        __ Mov(argc, actualNumArgs);
        PushArgsWithArgv(assembler, glue, argc, argV, argValue, currentSp, &invokeCompiledJSFunction, nullptr);
    }
    __ Bind(&invokeCompiledJSFunction);
    {
        __ Mov(Register(X19), expectedNumArgs);
        __ Str(actualNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE));
        __ Str(env, MemoryOperand(sp, 0)); // 0: means zero size
        __ Blr(codeAddr);
    }

    // pop argV argC
    // 3 : 3 means argC * 8
    __ Ldr(actualNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE));
    PopJSFunctionArgs(assembler, Register(X19), actualNumArgs);
    // pop prevLeaveFrameFp to restore thread->currentFrame_
    PopOptimizedArgsConfigFrame(assembler);
    __ Ret();
}

void OptimizedCall::OptimizedCallAsmInterpreter(ExtendedAssembler *assembler)
{
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_FROM_AOT);
    }
}

// * uint64_t CallBuiltinTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...)
// * webkit_jscc calling convention call runtime_id's runtime function(c-abi)
// * Argument:
//           %x0: glue
//
// * Construct Native Leave Frame Layout:
//          +--------------------------+
//          |       argv[N-1]          |
//          +--------------------------+
//          |      . . . . . .         |
//          +--------------------------+
//          |      argv[3]=a0          |
//          +--------------------------+
//          |      argv[2]=this        |
//          +--------------------------+
//          |   argv[1]=new-target     |
//          +--------------------------+
//          |   argv[0]=call-target    |
//          +--------------------------+ -----------------
//          |       argc               |                 ^
//          |--------------------------|                 |
//          |       env or thread      |                 |
//          |--------------------------|                 |
//          |       returnAddr         |    OptimizedBuiltinLeaveFrame
//  sp ---> |--------------------------|                 |
//          |       callsiteFp         |                 |
//          |--------------------------|                 |
//          |       frameType          |                 |
//          |--------------------------|                 |
//          |       align byte         |                 v
//          +--------------------------+ -----------------

void OptimizedCall::CallBuiltinTrampoline(ExtendedAssembler *assembler)
{
    Register glue(X0);
    Register sp(SP);
    Register nativeFuncAddr(X4);
    Register glueTemp(X2);
    Register temp(X1);
    Register zero(Zero);

    __ Mov(glueTemp, glue);
    __ Str(glue, MemoryOperand(sp, 0)); // thread (instead of env)
    __ Add(Register(X0), sp, Immediate(0));
    AsmInterpreterCall::PushBuiltinFrame(assembler, glueTemp, FrameType::BUILTIN_CALL_LEAVE_FRAME, temp, zero);

    AsmInterpreterCall::CallNativeInternal(assembler, nativeFuncAddr);
    __ Ret();
}

// * uint64_t JSCall(uintptr_t glue, JSTaggedType env, uint32_t argc, JSTaggedType calltarget, JSTaggedType new,
//                   JSTaggedType this, arg[0], arg[1], arg[2], ..., arg[N-1])
// * webkit_jscc calling convention call js function()
//
// * OptimizedJSFunctionFrame layout description as the following:
//               +--------------------------+
//               |        arg[N-1]          |
//               +--------------------------+
//               |       ...                |
//               +--------------------------+
//               |       arg[1]             |
//               +--------------------------+
//               |       arg[0]             |
//               +--------------------------+
//               |       this               |
//               +--------------------------+
//               |       new-target         |
//               +--------------------------+
//               |       call-target        |
//               |--------------------------|
//               |       argc               |
//               |--------------------------|
//               |       lexEnv             |
//      sp ----> |--------------------------| ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|   OptimizedJSFunctionFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------

void OptimizedCall::GenJSCall(ExtendedAssembler *assembler, bool isNew)
{
    Register jsfunc(X1);
    Register sp(SP);
    __ Ldr(jsfunc, MemoryOperand(sp, FRAME_SLOT_SIZE * 2)); // 2: skip env and argc
    JSCallInternal(assembler, jsfunc, isNew);
}

void OptimizedCall::JSCallNew(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallNew));
    GenJSCall(assembler, true);
}

void OptimizedCall::JSCall(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCall));
    GenJSCall(assembler, false);
}

void OptimizedCall::JSCallInternal(ExtendedAssembler *assembler, Register jsfunc, bool isNew)
{
    Register sp(SP);
    Register glue(X0);
    Register taggedValue(X2);
    Label nonCallable;
    Label notJSFunction;
    JSCallCheck(assembler, jsfunc, taggedValue, &nonCallable, &notJSFunction);

    Register method(X2);
    Register callField(X3);
    Register actualArgC(X4);
    Label callNativeMethod;
    Label callOptimizedMethod;
    Label lCallConstructor;
    Label lCallBuiltinStub;
    Label lCallNativeCpp;

    __ Ldr(Register(X5), MemoryOperand(jsfunc, JSFunction::LEXICAL_ENV_OFFSET));
    __ Str(Register(X5), MemoryOperand(sp, 0));
    __ Ldr(Register(X5), MemoryOperand(jsfunc, 0));
    __ Ldr(Register(X5), MemoryOperand(Register(X5), JSHClass::BIT_FIELD_OFFSET));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(actualArgC, MemoryOperand(sp, FRAME_SLOT_SIZE));
    __ Ldr(callField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Tbnz(callField, MethodLiteral::IsNativeBit::START_BIT, &callNativeMethod);
    if (!isNew) {
        __ Tbnz(Register(X5), JSHClass::ClassConstructorBit::START_BIT, &lCallConstructor);
    }
    __ Tbnz(callField, MethodLiteral::IsAotCodeBit::START_BIT, &callOptimizedMethod);
    {
        Register argV(X5);
        // aot argV = sp + 16
        __ Add(argV, sp, Immediate(DOUBLE_SLOT_SIZE));
        // asm interpreter argV = argv + 24
        __ Add(argV, argV, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE));
        __ Sub(actualArgC, actualArgC, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum()));
        OptimizedCallAsmInterpreter(assembler);
    }

    __ Bind(&callNativeMethod);
    {
        Register nativeFuncAddr(X4);
        if (!isNew) {
            __ Tbz(callField, MethodLiteral::IsFastBuiltinBit::START_BIT, &lCallNativeCpp);
            // 3 : 3 means call0 call1 call2 call3
            __ Cmp(actualArgC, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum() + 3));
            __ B(Condition::LE, &lCallBuiltinStub);
        } else {
            __ Tbnz(callField, MethodLiteral::IsFastBuiltinBit::START_BIT, &lCallBuiltinStub);
        }
        __ Bind(&lCallNativeCpp);
        __ Ldr(nativeFuncAddr, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        CallBuiltinTrampoline(assembler);
    }

    __ Bind(&lCallBuiltinStub);
    {
        TempRegister1Scope scope1(assembler);
        Register builtinStub = __ TempRegister1();
        __ Ldr(Register(X5), MemoryOperand(method, Method::EXTRA_LITERAL_INFO_OFFSET));  // get extra literal
        __ And(Register(X5).W(), Register(X5).W(), LogicalImmediate::Create(0xff, RegWSize));
        if (!isNew) {
            __ Cmp(Register(X5).W(), Immediate(kungfu::BuiltinsStubCSigns::BUILTINS_CONSTRUCTOR_STUB_FIRST));
            __ B(Condition::GE, &lCallNativeCpp);
        }
        __ Add(builtinStub, glue, Operand(Register(X5).W(), UXTW, FRAME_SLOT_SIZE_LOG2));
        __ Ldr(builtinStub, MemoryOperand(builtinStub, JSThread::GlueData::GetBuiltinsStubEntriesOffset(false)));

        __ Ldr(Register(X1), MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        __ Ldr(Register(X2), MemoryOperand(sp, DOUBLE_SLOT_SIZE));  // get jsfunc
        __ Ldr(Register(X3), MemoryOperand(sp, TRIPLE_SLOT_SIZE));  // get newtarget
        __ Ldr(Register(X4), MemoryOperand(sp, QUADRUPLE_SLOT_SIZE));  // get this
        __ Ldr(Register(X5), MemoryOperand(sp, FRAME_SLOT_SIZE));  // get number args
        __ Sub(Register(X5), Register(X5), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        if (!isNew) {
            Label lTailCall;
            Register fp(X29);
            __ Ldp(Register(X6), Register(X7), MemoryOperand(sp, QUINTUPLE_SLOT_SIZE));  // get arg0 arg1
            __ Cmp(Register(X5), Immediate(3));  // 3: callarg3
            __ B(Condition::NE, &lTailCall);
            PushAsmBridgeFrame(assembler);
            {
                // push arg3 and call
                TempRegister2Scope scope2(assembler);
                Register arg3 = __ TempRegister2();
                __ Ldr(arg3, MemoryOperand(fp, DECUPLE_SLOT_SIZE)); // get arg2
                __ Stp(arg3, Register(X8), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
                __ Blr(builtinStub);
                __ Add(sp, sp, Immediate(DOUBLE_SLOT_SIZE));
            }
            PopOptimizedFrame(assembler);
            __ Ret();
            __ Bind(&lTailCall);
            {
                __ Br(builtinStub);
            }
        } else {
            __ Add(Register(X6), sp, Immediate(QUINTUPLE_SLOT_SIZE));  // get argV
            __ Br(builtinStub);
        }
    }

    __ Bind(&callOptimizedMethod);
    {
        CallOptimziedMethodInternal(assembler, jsfunc, actualArgC, callField, sp);
    }
    Label jsBoundFunction;
    Label jsProxy;
    __ Bind(&notJSFunction);
    {
        Register bitfield(X2);
        Register jstype2(X5, W);
        __ And(jstype2, bitfield.W(), LogicalImmediate::Create(0xff, RegWSize));
        __ Cmp(jstype2, Immediate(static_cast<int64_t>(JSType::JS_BOUND_FUNCTION)));
        __ B(Condition::EQ, &jsBoundFunction);
        __ Cmp(jstype2, Immediate(static_cast<int64_t>(JSType::JS_PROXY)));
        __ B(Condition::EQ, &jsProxy);
        __ Ret();
    }

    __ Bind(&jsBoundFunction);
    {
        JSBoundFunctionCallInternal(assembler, glue, actualArgC, jsfunc, RTSTUB_ID(JSCall));
    }
    __ Bind(&jsProxy);
    {
        JSProxyCallInternal(assembler, sp, jsfunc);
    }
    __ Bind(&nonCallable);
    {
        ThrowNonCallableInternal(assembler, sp);
    }
    __ Bind(&lCallConstructor);
    {
        Register frameType(X6);
        __ PushFpAndLr();
        __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
        // 2 : 2 means pair
        __ Stp(Register(Zero), frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Add(Register(FP), sp, Immediate(DOUBLE_SLOT_SIZE));
        Register argC(X5);
        Register runtimeId(X6);
        __ Mov(argC, Immediate(0));
        __ Mov(runtimeId, Immediate(RTSTUB_ID(ThrowCallConstructorException)));
        // 2 : 2 means pair
        __ Stp(runtimeId, argC, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
        // 4 : 4 means stack slot
        __ Add(sp, sp, Immediate(4 * FRAME_SLOT_SIZE));
        __ RestoreFpAndLr();
        __ Ret();
    }
}

void OptimizedCall::ConstructorJSCall([[maybe_unused]]ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ConstructorJSCall));
    Register jsfunc(X1);
    Register sp(SP);
    __ Ldr(jsfunc, MemoryOperand(sp, FRAME_SLOT_SIZE * 2)); // 2: skip env and argc
    ConstructorJSCallInternal(assembler, jsfunc);
}

void OptimizedCall::ConstructorJSCallInternal(ExtendedAssembler *assembler, Register jsfunc)
{
    Register sp(SP);
    Register glue(X0);
    Register taggedValue(X2);
    Label nonCallable;
    Label notJSFunction;
    JSCallCheck(assembler, jsfunc, taggedValue, &nonCallable, &notJSFunction);

    Register method(X2);
    Register callField(X3);
    Register actualArgC(X4);
    Label callNativeMethod;
    Label callOptimizedMethod;
    __ Ldr(Register(X5), MemoryOperand(jsfunc, JSFunction::LEXICAL_ENV_OFFSET));
    __ Str(Register(X5), MemoryOperand(sp, 0));
    __ Ldr(Register(X5), MemoryOperand(jsfunc, JSFunction::HCLASS_OFFSET));
    __ Ldr(Register(X5), MemoryOperand(Register(X5), JSHClass::BIT_FIELD_OFFSET));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(actualArgC, MemoryOperand(sp, FRAME_SLOT_SIZE));
    __ Ldr(callField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Tbnz(callField, MethodLiteral::IsNativeBit::START_BIT, &callNativeMethod);
    __ Tbnz(callField, MethodLiteral::IsAotCodeBit::START_BIT, &callOptimizedMethod);
    {
        Register argV(X5);
        // aot argV = sp + 16
        __ Add(argV, sp, Immediate(DOUBLE_SLOT_SIZE));
        // asm interpreter argV = argv + 24
        __ Add(argV, argV, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE));
        OptimizedCallAsmInterpreter(assembler);
    }

    __ Bind(&callNativeMethod);
    {
        Register nativeFuncAddr(X4);
        __ Ldr(nativeFuncAddr, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        CallBuiltinTrampoline(assembler);
    }

    __ Bind(&callOptimizedMethod);
    {
        CallOptimziedMethodInternal(assembler, jsfunc, actualArgC, callField, sp);
    }
    Label jsBoundFunction;
    Label jsProxy;
    __ Bind(&notJSFunction);
    {
        Register bitfield(X2);
        Register jstype2(X5, W);
        __ And(jstype2, bitfield.W(), LogicalImmediate::Create(0xff, RegWSize));
        __ Cmp(jstype2, Immediate(static_cast<int64_t>(JSType::JS_BOUND_FUNCTION)));
        __ B(Condition::EQ, &jsBoundFunction);
        __ Cmp(jstype2, Immediate(static_cast<int64_t>(JSType::JS_PROXY)));
        __ B(Condition::EQ, &jsProxy);
        __ Ret();
    }

    __ Bind(&jsBoundFunction);
    {
        JSBoundFunctionCallInternal(assembler, glue, actualArgC, jsfunc, RTSTUB_ID(ConstructorJSCall));
    }
    __ Bind(&jsProxy);
    {
        JSProxyCallInternal(assembler, sp, jsfunc);
    }
    __ Bind(&nonCallable);
    {
        ThrowNonCallableInternal(assembler, sp);
    }
}

void OptimizedCall::JSCallCheck(ExtendedAssembler *assembler, Register jsfunc, Register taggedValue,
                                Label *nonCallable, Label *notJSFunction)
{
    __ Mov(taggedValue, JSTaggedValue::TAG_MARK);
    __ Cmp(jsfunc, taggedValue);
    __ B(Condition::HS, nonCallable);
    __ Cbz(jsfunc, nonCallable);
    __ Mov(taggedValue, JSTaggedValue::TAG_SPECIAL);
    __ And(taggedValue, jsfunc, taggedValue);
    __ Cbnz(taggedValue, nonCallable);

    Register jshclass(X2);
    __ Ldr(jshclass, MemoryOperand(jsfunc, JSFunction::HCLASS_OFFSET));
    Register bitfield(X2);
    __ Ldr(bitfield, MemoryOperand(jshclass, JSHClass::BIT_FIELD_OFFSET));
    __ Tbz(bitfield, JSHClass::CallableBit::START_BIT, nonCallable);

    Register jstype(X3, W);
    __ And(jstype, bitfield, LogicalImmediate::Create(0xFF, RegWSize));
    // 4 : 4 means JSType::JS_FUNCTION_FIRST
    __ Sub(jstype, jstype, Immediate(static_cast<int>(JSType::JS_FUNCTION_FIRST)));
    // 9 : 9 means JSType::JS_FUNCTION_LAST - JSType::JS_FUNCTION_FIRST + 1
    __ Cmp(jstype, Immediate(static_cast<int>(JSType::JS_FUNCTION_LAST)
            - static_cast<int>(JSType::JS_FUNCTION_FIRST) + 1));
    __ B(Condition::HS, notJSFunction);
}

void OptimizedCall::ThrowNonCallableInternal(ExtendedAssembler *assembler, Register sp)
{
    Register frameType(X6);
    Register taggedMessageId(X5);
    __ PushFpAndLr();
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
    __ Mov(taggedMessageId,
        Immediate(JSTaggedValue(GET_MESSAGE_STRING_ID(NonCallable)).GetRawData()));
    // 2 : 2 means pair
    __ Stp(taggedMessageId, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(Register(FP), sp, Immediate(DOUBLE_SLOT_SIZE));
    Register argC(X5);
    Register runtimeId(X6);
    __ Mov(argC, Immediate(1));
    __ Mov(runtimeId, Immediate(RTSTUB_ID(ThrowTypeError)));
    // 2 : 2 means pair
    __ Stp(runtimeId, argC, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
    __ Mov(Register(X0), Immediate(JSTaggedValue::VALUE_EXCEPTION));
    // 4 : 4 means stack slot
    __ Add(sp, sp, Immediate(4 * FRAME_SLOT_SIZE));
    __ RestoreFpAndLr();
    __ Ret();
}

void OptimizedCall::CallOptimziedMethodInternal(ExtendedAssembler *assembler, Register jsfunc, Register actualArgC,
                                                Register callField, Register sp)
{
    Register expectedNumArgs(X1, W);
    Register arg2(X2);
    Register codeAddress(X3);
    Register argV(X4);
    Register env(X5);
    Register method(X6);
    Label directCallCodeEntry;
    const int64_t argoffsetSlot = static_cast<int64_t>(CommonArgIdx::FUNC) - 1;
    __ Mov(Register(X5), jsfunc);
    __ Mov(arg2, actualArgC);
    __ Lsr(callField, callField, MethodLiteral::NumArgsBits::START_BIT);
    __ And(callField.W(), callField.W(),
        LogicalImmediate::Create(
            MethodLiteral::NumArgsBits::Mask() >> MethodLiteral::NumArgsBits::START_BIT, RegWSize));
    __ Add(expectedNumArgs, callField.W(), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Cmp(arg2.W(), expectedNumArgs);
    __ Add(argV, sp, Immediate(argoffsetSlot * FRAME_SLOT_SIZE));  // skip env and numArgs
    __ Ldr(method, MemoryOperand(Register(X5), JSFunctionBase::METHOD_OFFSET)); // get method
    __ Ldr(codeAddress, MemoryOperand(method, Method::CODE_ENTRY_OFFSET)); // get codeAddress
    __ Ldr(env, MemoryOperand(sp, 0));
    __ B(Condition::HS, &directCallCodeEntry);
    __ CallAssemblerStub(RTSTUB_ID(OptimizedCallOptimized), true);
    __ Bind(&directCallCodeEntry);
    __ Br(codeAddress);
}

void OptimizedCall::JSBoundFunctionCallInternal(ExtendedAssembler *assembler, Register glue,
                                                Register actualArgC, Register jsfunc, int stubId)
{
    // construct frame
    PushOptimizedArgsConfigFrame(assembler);
    Register basefp(X29);
    Register fp = __ AvailableRegister1();
    Register env(X5);

    Register argV(X6);
    __ Add(argV, basefp, Immediate(GetStackArgOffSetToFp(0))); // 0: first index id
    __ Ldr(actualArgC, MemoryOperand(argV, FRAME_SLOT_SIZE));
    __ Ldr(env, MemoryOperand(argV, 0));

    Register boundLength(X2);
    Register realArgC(X7, W);
    Label copyBoundArgument;
    Label pushCallTarget;
    // get bound arguments
    __ Ldr(boundLength, MemoryOperand(jsfunc, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
    //  get bound length
    __ Ldr(boundLength, MemoryOperand(boundLength, TaggedArray::LENGTH_OFFSET));
    __ Add(realArgC, boundLength.W(), actualArgC.W());
    __ Mov(Register(X19), realArgC);
    IncreaseStackForArguments(assembler, realArgC, fp);
    __ Sub(actualArgC.W(), actualArgC.W(), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Cmp(actualArgC.W(), Immediate(0));
    __ B(Condition::EQ, &copyBoundArgument);
    {
        TempRegister1Scope scope1(assembler);
        Register tmp = __ TempRegister1();
        const int64_t argoffsetSlot = static_cast<int64_t>(CommonArgIdx::FUNC) - 1;
        __ Add(argV, argV, Immediate((NUM_MANDATORY_JSFUNC_ARGS + argoffsetSlot) *FRAME_SLOT_SIZE));
        PushArgsWithArgv(assembler, glue, actualArgC, argV, tmp, fp, nullptr, nullptr);
    }
    __ Bind(&copyBoundArgument);
    {
        Register boundArgs(X4);
        __ Ldr(boundArgs, MemoryOperand(jsfunc, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
        __ Add(boundArgs, boundArgs, Immediate(TaggedArray::DATA_OFFSET));
        __ Cmp(boundLength.W(), Immediate(0));
        __ B(Condition::EQ, &pushCallTarget);
        {
            TempRegister1Scope scope1(assembler);
            Register tmp = __ TempRegister1();
            PushArgsWithArgv(assembler, glue, boundLength, boundArgs, tmp, fp, nullptr, nullptr);
        }
    }
    __ Bind(&pushCallTarget);
    {
        Register thisObj(X4);
        Register newTarget(X6);
        Register boundTarget(X7);
        __ Ldr(thisObj, MemoryOperand(jsfunc, JSBoundFunction::BOUND_THIS_OFFSET));
        __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        // 2 : 2 means pair
        __ Stp(newTarget, thisObj, MemoryOperand(fp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Ldr(boundTarget, MemoryOperand(jsfunc, JSBoundFunction::BOUND_TARGET_OFFSET));
        // 2 : 2 means pair
        __ Stp(Register(X19), boundTarget, MemoryOperand(fp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Str(env, MemoryOperand(fp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ CallAssemblerStub(stubId, false);

    PopJSFunctionArgs(assembler, Register(X19), Register(X19));
    PopOptimizedArgsConfigFrame(assembler);
    __ Ret();
}

void OptimizedCall::JSProxyCallInternal(ExtendedAssembler *assembler, Register sp, Register jsfunc)
{
    // input: x1(calltarget)
    // output: glue:x0 argc:x1 calltarget:x2 argv:x3
    __ Mov(Register(X2), jsfunc);
    __ Ldr(Register(X1), MemoryOperand(sp, FRAME_SLOT_SIZE)); // 8: skip env
    __ Add(X3, sp, Immediate(FRAME_SLOT_SIZE * 2)); // 2: get argv

    Register proxyCallInternalId(X9);
    Register baseAddress(X8);
    Register codeAddress(X10);
    __ Mov(baseAddress, Immediate(JSThread::GlueData::GetCOStubEntriesOffset(false)));
    __ Mov(proxyCallInternalId, Immediate(CommonStubCSigns::JsProxyCallInternal));
    __ Add(codeAddress, X0, baseAddress);
    __ Ldr(codeAddress, MemoryOperand(codeAddress, proxyCallInternalId, UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Br(codeAddress);
}

// * uint64_t JSProxyCallInternalWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, uintptr_t argv[])
// * c++ calling convention call js function
// * Arguments:
//        %x0 - glue
//        %x1 - argc
//        %x2 - calltarget
//        %x3 - argV[] = { calltarget, newtarget, thisObj, arg[0], arg[1], ..., arg[N-1])

void OptimizedCall::JSProxyCallInternalWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSProxyCallInternalWithArgV));
    Register jsfunc(X1);
    Register argv(X3);
    __ Mov(jsfunc, Register(X2));
    __ Str(jsfunc, MemoryOperand(argv, 0));
    JSCallInternal(assembler, jsfunc);
}

// * uint64_t CallRuntimeWithArgv(uintptr_t glue, uint64_t runtime_id, uint64_t argc, uintptr_t argv)
// * cc calling convention call runtime_id's runtion function(c-abi)
// * Arguments:
//         %x0 - glue
//         %x1 - runtime_id
//         %x2 - argc
//         %x3 - argv
//
// * Optimized-leaved-frame-with-argv layout as the following:
//         +--------------------------+
//         |       argv[]             |
//         +--------------------------+-------------
//         |       argc               |            ^
//         |--------------------------|            |
//         |       RuntimeId          |   OptimizedWithArgvLeaveFrame
//  sp --> |--------------------------|            |
//         |       returnAddr         |            |
//         |--------------------------|            |
//         |       callsiteFp         |            |
//         |--------------------------|            |
//         |       frameType          |            v
//         +--------------------------+-------------

void OptimizedCall::CallRuntimeWithArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallRuntimeWithArgv));
    Register glue(X0);
    Register runtimeId(X1);
    Register argc(X2);
    Register argv(X3);
    Register sp(SP);
    // 2 : 2 means pair
    __ Stp(argc, argv, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Str(runtimeId, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ PushFpAndLr();
    Register fp(X29);
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    // construct leave frame
    Register frameType(X9);
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME_WITH_ARGV)));
    __ Str(frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(Register(FP), sp, Immediate(FRAME_SLOT_SIZE));

     // load runtime trampoline address
    Register tmp(X9);
    Register rtfunc(X9);
    // 3 : 3 means 2 << 3 = 8
    __ Add(tmp, glue, Operand(runtimeId, LSL, 3));
    __ Ldr(rtfunc, MemoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Mov(X1, argc);
    __ Mov(X2, argv);
    __ Blr(rtfunc);
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ RestoreFpAndLr();
    __ Add(sp, sp, Immediate(3 * FRAME_SLOT_SIZE)); // 3 : 3 means pair
    __ Ret();
}

void OptimizedCall::PushMandatoryJSArgs(ExtendedAssembler *assembler, Register jsfunc,
                                        Register thisObj, Register newTarget, Register currentSp)
{
    __ Str(thisObj, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Str(newTarget, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Str(jsfunc, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
}

void OptimizedCall::PopJSFunctionArgs(ExtendedAssembler *assembler, Register expectedNumArgs, Register actualNumArgs)
{
    Register sp(SP);
    Register fp(X6);
    Label aligned;
    const int64_t argoffsetSlot = static_cast<int64_t>(CommonArgIdx::FUNC) - 1;
    if (expectedNumArgs != actualNumArgs) {
        TempRegister1Scope scop1(assembler);
        Register tmp = __ TempRegister1();
        __ Cmp(expectedNumArgs, actualNumArgs);
        __ CMov(tmp, expectedNumArgs, actualNumArgs, Condition::HI);
        __ Add(sp, sp, Operand(tmp, UXTW, FRAME_SLOT_SIZE_LOG2));
    } else {
        __ Add(sp, sp, Operand(expectedNumArgs, UXTW, FRAME_SLOT_SIZE_LOG2));
    }
    __ Add(sp, sp, Immediate(argoffsetSlot * FRAME_SLOT_SIZE));
    __ Mov(fp, sp);
    __ Tst(fp, LogicalImmediate::Create(0xf, RegXSize));  // 0xf: 0x1111
    __ B(Condition::EQ, &aligned);
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ Bind(&aligned);
}

void OptimizedCall::PushJSFunctionEntryFrame (ExtendedAssembler *assembler, Register prevFp)
{
    Register fp(X29);
    Register sp(SP);
    TempRegister2Scope temp2Scope(assembler);
    __ PushFpAndLr();
    Register frameType = __ TempRegister2();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_ENTRY_FRAME)));
    // 2 : 2 means pairs
    __ Stp(prevFp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(Register(FP), sp, Immediate(DOUBLE_SLOT_SIZE));
    __ CalleeSave();
}

void OptimizedCall::PopJSFunctionEntryFrame(ExtendedAssembler *assembler, Register glue)
{
    Register fp(X29);
    Register sp(SP);
    Register prevFp(X1);
    __ CalleeRestore();

    // 2: prevFp and frameType
    __ Ldp(prevFp, Register(Zero), MemoryOperand(sp, FRAME_SLOT_SIZE * 2, AddrMode::POSTINDEX));
    // restore return address
    __ RestoreFpAndLr();
    __ Str(prevFp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
}

void OptimizedCall::PushOptimizedArgsConfigFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    __ PushFpAndLr();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
    // 2 : 2 means pairs. X19 means calleesave and 16bytes align
    __ Stp(Register(X19), frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(Register(FP), sp, Immediate(DOUBLE_SLOT_SIZE));
}

void OptimizedCall::PopOptimizedArgsConfigFrame(ExtendedAssembler *assembler)
{
    TempRegister2Scope temp2Scope(assembler);
    Register sp(SP);
    Register frameType = __ TempRegister2();
    // 2 : 2 means pop call site sp and type
    __ Ldp(Register(X19), frameType, MemoryOperand(sp, FRAME_SLOT_SIZE * 2, AddrMode::POSTINDEX));
    __ RestoreFpAndLr();
}

void OptimizedCall::PushAsmBridgeFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    __ PushFpAndLr();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::ASM_BRIDGE_FRAME)));
    // 2 : 2 means pairs. X19 means calleesave and 16bytes align
    __ Stp(Register(X19), frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(Register(FP), sp, Immediate(FRAME_SLOT_SIZE));
}

void OptimizedCall::PopOptimizedFrame(ExtendedAssembler *assembler)
{
    TempRegister2Scope temp2Scope(assembler);
    Register sp(SP);
    Register frameType = __ TempRegister2();
    // 2 : 2 means pop call site sp and type
    __ Ldp(Register(X19), frameType, MemoryOperand(sp, FRAME_SLOT_SIZE * 2, AddrMode::POSTINDEX));
    __ RestoreFpAndLr();
}

// * uint64_t PushOptimizedUnfoldArgVFrame(uintptr_t glue, uint32_t argc, JSTaggedType calltarget,
//                                         JSTaggedType new, JSTaggedType this, JSTaggedType argV[])
// * cc calling convention call js function()
// * arguments:
//              %x0 - glue
//              %x1 - argc
//              %x2 - call-target
//              %x3 - new-target
//              %x4 - this
//              %x5 - argv
//
// * OptimizedUnfoldArgVFrame layout description as the following:
//      sp ----> |--------------------------| ---------------
//               |       returnAddr         |               ^
//  currentFp--> |--------------------------|               |
//               |       prevFp             |               |
//               |--------------------------|   OptimizedUnfoldArgVFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       currentFp          |               v
//               +--------------------------+ ---------------

void OptimizedCall::PushOptimizedUnfoldArgVFrame(ExtendedAssembler *assembler, Register callSiteSp)
{
    Register sp(SP);
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    __ PushFpAndLr();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME)));
    // 2 : 2 means pairs
    __ Stp(callSiteSp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(Register(FP), sp, Immediate(DOUBLE_SLOT_SIZE));
}

void OptimizedCall::PopOptimizedUnfoldArgVFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);
    // 2 : 2 means pop call site sp and type
    __ Add(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    __ RestoreFpAndLr();
}

// * uint64_t JSCallWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType calltarget,
//                          JSTaggedType new, JSTaggedType this, argV)
// * cc calling convention call js function()
// * arguments:
//              %x0 - glue
//              %x1 - argc
//              %x2 - call-target
//              %x3 - new-target
//              %x4 - this
//              %x5 - argV[]
//
// * OptimizedJSFunctionFrame layout description as the following:
//               +--------------------------+
//               |        arg[N-1]          |
//               +--------------------------+
//               |        . . . . .         |
//               +--------------------------+
//               |        arg[0]            |
//               +--------------------------+
//               |       this               |
//               +--------------------------+
//               |       new-target         |
//               +--------------------------+
//               |       call-target        |
//               |--------------------------|
//               |       argc               |
//      sp ----> |--------------------------| ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|  OptimizedJSFunctionFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------

void OptimizedCall::GenJSCallWithArgV(ExtendedAssembler *assembler, bool isNew)
{
    Register sp(SP);
    Register glue(X0);
    Register actualNumArgs(X1);
    Register jsfunc(X2);
    Register newTarget(X3);
    Register thisObj(X4);
    Register argV(X5);
    Register env(X6);
    Register currentSp = __ AvailableRegister1();
    Register callsiteSp = __ AvailableRegister2();
    Label pushCallThis;

    __ Mov(callsiteSp, sp);
    PushOptimizedUnfoldArgVFrame(assembler, callsiteSp);
    Register argC(X7);
    __ Add(actualNumArgs, actualNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Mov(argC, actualNumArgs);
    IncreaseStackForArguments(assembler, argC, currentSp);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register tmp = __ TempRegister1();
        Register op = __ TempRegister2();
        __ Sub(tmp, actualNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        PushArgsWithArgv(assembler, glue, tmp, argV, op, currentSp, &pushCallThis, nullptr);
    }
    __ Bind(&pushCallThis);
    PushMandatoryJSArgs(assembler, jsfunc, thisObj, newTarget, currentSp);
    __ Str(actualNumArgs, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(env, MemoryOperand(jsfunc, JSFunction::LEXICAL_ENV_OFFSET));
    __ Str(env, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    if (isNew) {
        __ CallAssemblerStub(RTSTUB_ID(JSCallNew), false);
    } else {
        __ CallAssemblerStub(RTSTUB_ID(JSCall), false);
    }

    __ Ldr(actualNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE));
    PopJSFunctionArgs(assembler, actualNumArgs, actualNumArgs);
    PopOptimizedUnfoldArgVFrame(assembler);
    __ Ret();
}

void OptimizedCall::JSCallNewWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallNewWithArgV));
    GenJSCallWithArgV(assembler, true);
}

void OptimizedCall::JSCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallWithArgV));
    GenJSCallWithArgV(assembler, false);
}

void OptimizedCall::ConstructorJSCallWithArgV([[maybe_unused]]ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ConstructorJSCallWithArgV));
    Register sp(SP);
    Register glue(X0);
    Register actualNumArgs(X1);
    Register jsfunc(X2);
    Register newTarget(X3);
    Register thisObj(X4);
    Register argV(X5);
    Register env(X6);
    Register currentSp = __ AvailableRegister1();
    Register callsiteSp = __ AvailableRegister2();
    Label pushCallThis;

    __ Mov(callsiteSp, sp);
    PushOptimizedUnfoldArgVFrame(assembler, callsiteSp);
    Register argC(X7);
    __ Add(actualNumArgs, actualNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Mov(argC, actualNumArgs);
    IncreaseStackForArguments(assembler, argC, currentSp);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register tmp = __ TempRegister1();
        Register op = __ TempRegister2();
        __ Sub(tmp, actualNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        PushArgsWithArgv(assembler, glue, tmp, argV, op, currentSp, &pushCallThis, nullptr);
    }
    __ Bind(&pushCallThis);
    PushMandatoryJSArgs(assembler, jsfunc, thisObj, newTarget, currentSp);
    __ Str(actualNumArgs, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(env, MemoryOperand(jsfunc, JSFunction::LEXICAL_ENV_OFFSET));
    __ Str(env, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    __ CallAssemblerStub(RTSTUB_ID(ConstructorJSCall), false);

    __ Ldr(actualNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE));
    PopJSFunctionArgs(assembler, actualNumArgs, actualNumArgs);
    PopOptimizedUnfoldArgVFrame(assembler);
    __ Ret();
}

void OptimizedCall::DeoptEnterAsmInterp(ExtendedAssembler *assembler)
{
    // rdi
    Register glueRegister = __ GlueRegister();
    Register context(X2);
    Register opRegister(X9);
    Register outputCount(X10);
    Register frameStateBase(X11);
    Register currentSlotRegister(X12);
    Register sp(SP);

    __ PushFpAndLr();

    __ Mov(currentSlotRegister, sp);
    __ Ldr(outputCount, MemoryOperand(context, AsmStackContext::GetOutputCountOffset(false)));
    __ Add(frameStateBase, context, Immediate(AsmStackContext::GetSize(false)));

    Label stackOverflow;
    // update fp
    __ Str(currentSlotRegister, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetFpOffset(false)));
    PushArgsWithArgv(assembler, glueRegister, outputCount, frameStateBase, opRegister,
                     currentSlotRegister, nullptr, &stackOverflow);

    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);
    {
        // X19, fp, x20, x21,      x22,     x23,  x24
        // glue sp   pc  constpool  profile  acc   hotness
        __ Ldr(callTargetRegister, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetFunctionOffset(false)));
        __ Ldr(Register(X20), MemoryOperand(frameStateBase, AsmInterpretedFrame::GetPcOffset(false)));
        __ Ldr(Register(X23), MemoryOperand(frameStateBase, AsmInterpretedFrame::GetAccOffset(false)));
        __ Ldr(methodRegister, MemoryOperand(callTargetRegister, JSFunctionBase::METHOD_OFFSET));

        __ Add(opRegister, currentSlotRegister, Immediate(AsmInterpretedFrame::GetSize(false)));

        __ Align16(currentSlotRegister);
        __ Mov(Register(SP), currentSlotRegister);
        AsmInterpreterCall::DispatchCall(assembler, Register(X20), opRegister, Register(X23));
    }
    __ Bind(&stackOverflow);
    {
        Register temp(X1);
        AsmInterpreterCall::ThrowStackOverflowExceptionAndReturn(
            assembler, glueRegister, sp, temp);
    }
}

void OptimizedCall::DeoptHandlerAsm(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(DeoptHandlerAsm));
    __ PushFpAndLr();
    Register sp(SP);
    Register fp(FP);
    Register frameType(X2);
    Register glueReg(X0);

    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::ASM_BRIDGE_FRAME)));
    __ Stp(glueReg, frameType, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
    __ CalleeSave();

    Register deoptType(X1);
    Register align(X3);
    Register argC(X3);
    Register runtimeId(X2);
    __ Stp(deoptType, align, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(argC, Immediate(1));
    __ Mov(runtimeId, Immediate(RTSTUB_ID(DeoptHandler)));
    __ Stp(runtimeId, argC, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
    __ Add(sp, sp, Immediate(2 * DOUBLE_SLOT_SIZE)); // 2: skip runtimeId, argc, deoptType, align

    __ CalleeRestore();
    Register context(X2);
    __ Mov(context, Register(X0));
    __ Ldr(glueReg, MemoryOperand(sp, 0));

    Register ret(X0);
    Label stackOverflow;
    __ Cmp(ret, Immediate(JSTaggedValue::VALUE_EXCEPTION));
    __ B(Condition::EQ, &stackOverflow);

    Label target;
    Register temp(X1);
    __ Ldr(fp, MemoryOperand(context, AsmStackContext::GetCallerFpOffset(false)));
    __ Ldr(temp, MemoryOperand(context, AsmStackContext::GetCallFrameTopOffset(false)));
    __ Mov(sp, temp);
    __ Ldr(Register(X30), MemoryOperand(context, AsmStackContext::GetReturnAddressOffset(false)));

    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    DeoptEnterAsmInterp(assembler);

    __ Bind(&stackOverflow);
    {
        __ Mov(runtimeId, Immediate(RTSTUB_ID(ThrowStackOverflowException)));
        // 2 : 2 means pair
        __ Stp(runtimeId, Register(Zero), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
        __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
        __ Add(sp, sp, Immediate(2 * DOUBLE_SLOT_SIZE)); // 2: skip runtimeId&argc glue&type
        __ RestoreFpAndLr();
        __ Ret();
    }
}
#undef __
}  // panda::ecmascript::aarch64