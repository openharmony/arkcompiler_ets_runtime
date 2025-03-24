/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/message_string.h"

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
    __ Add(fp, sp, Immediate(2 * FRAME_SLOT_SIZE));  // 2 : 2 means pairs
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

void OptimizedCall::IncreaseStackForArguments(ExtendedAssembler *assembler, Register argc, Register currentSp,
                                              int64_t numExtraArgs)
{
    Register sp(SP);
    __ Mov(currentSp, sp);
    if (numExtraArgs > 0) {
        // add extra aguments, numArgs
        __ Add(argc, argc, Immediate(numExtraArgs));
    }
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
//        %x4 - needPushArgv
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
    Register needPushArgv(X4);
    Register sp(SP);
    Register tmpArgV(X7);
    Label lJSCallWithArgVAndPushArgv;
    Label lPopFrame;

    PushJSFunctionEntryFrame (assembler, prevFpReg);
    __ Mov(Register(X6), needPushArgv);
    __ Mov(tmpArgV, argV);
    __ Mov(Register(X20), glueReg);
    __ Ldr(Register(X2), MemoryOperand(tmpArgV, 0));
    __ Ldr(Register(X3), MemoryOperand(tmpArgV, FRAME_SLOT_SIZE));
    __ Ldr(Register(X4), MemoryOperand(tmpArgV, DOUBLE_SLOT_SIZE));
    __ Add(tmpArgV, tmpArgV, Immediate(TRIPLE_SLOT_SIZE));
    __ Mov(Register(X5), tmpArgV);
    __ Cmp(Register(X6), Immediate(1));
    __ B(Condition::EQ, &lJSCallWithArgVAndPushArgv);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgV), false);
    __ B(&lPopFrame);

    __ Bind(&lJSCallWithArgVAndPushArgv);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgVAndPushArgv), false);
    __ Bind(&lPopFrame);
    __ Mov(Register(X2), Register(X20));
    PopJSFunctionEntryFrame(assembler, Register(X2));
    __ Ret();
}

// * uint64_t OptimizedCallAndPushArgv(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, JSTaggedType new,
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
//               +--------------------------+
//               |       argv               |
//               +--------------------------+
//               |       argc               |
//     sp ---->  |--------------------------| ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|   OptimizedJSFunctionFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------
void OptimizedCall::OptimizedCallAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedCallAndPushArgv));
    Register sp(SP);
    Register jsfunc(X7);
    Register method(X6);
    Register expectedNumArgs(X1);
    Register actualNumArgs(X2);
    Register codeAddr(X3);
    Register argV(X4);

    auto funcSlotOffSet = kungfu::ArgumentAccessor::GetExtraArgsNum();
    __ Ldr(jsfunc, MemoryOperand(sp, funcSlotOffSet * FRAME_SLOT_SIZE));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(codeAddr, MemoryOperand(jsfunc, JSFunction::CODE_ENTRY_OFFSET));
    __ Ldr(expectedNumArgs, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Lsr(expectedNumArgs, expectedNumArgs, MethodLiteral::NumArgsBits::START_BIT);
    __ And(expectedNumArgs, expectedNumArgs,
        LogicalImmediate::Create(
            MethodLiteral::NumArgsBits::Mask() >> MethodLiteral::NumArgsBits::START_BIT, RegXSize));
    __ Add(expectedNumArgs, expectedNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));

    __ Add(argV, sp, Immediate(funcSlotOffSet * FRAME_SLOT_SIZE));  // skip numArgs and argv
    __ Ldr(actualNumArgs, MemoryOperand(sp, 0));

    Register glue(X0);
    Register currentSp(X5);
    Label copyArguments;
    Label invokeCompiledJSFunction;

    // construct frame
    PushOptimizedArgsConfigFrame(assembler);
    Register argC(X7);
    __ Cmp(expectedNumArgs, actualNumArgs);
    __ CMov(argC, expectedNumArgs, actualNumArgs, Condition::HI);
    IncreaseStackForArguments(assembler, argC, currentSp, static_cast<int64_t>(CommonArgIdx::ACTUAL_ARGV));
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
        __ Str(currentSp, MemoryOperand(sp, FRAME_SLOT_SIZE));
        __ Str(actualNumArgs, MemoryOperand(sp, 0)); // argv, argc
        __ Blr(codeAddr);
    }

    // pop argV argC
    // 3 : 3 means argC * 8
    __ Ldr(actualNumArgs, MemoryOperand(sp, 0));
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
        AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_FROM_AOT,
                                              FrameTransitionType::OTHER_TO_OTHER);
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
//          |       thread             |                 |
//          |--------------------------|                 |
//          |       returnAddr         |    OptimizedBuiltinLeaveFrame
//  sp ---> |--------------------------|                 |
//          |       callsiteFp         |                 |
//          |--------------------------|                 |
//          |       frameType          |                 v
//          +--------------------------+ -----------------

void OptimizedCall::CallBuiltinTrampoline(ExtendedAssembler *assembler)
{
    Register glue(X0);
    Register sp(SP);
    Register nativeFuncAddr(X4);
    Register temp(X1);

    // remove argv
    __ Ldr(temp, MemoryOperand(sp, 0));
    __ Stp(glue, temp, MemoryOperand(sp, 0));   // argc, glue
    // returnAddr, callsiteFp
    __ Stp(Register(X29), Register(X30), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(temp, sp);
    __ Str(temp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false))); // rbp
    __ Mov(Register(X29), temp); // rbp
    __ Mov(temp, Immediate(static_cast<int32_t>(FrameType::BUILTIN_CALL_LEAVE_FRAME)));
    __ Stp(Register(Zero), temp, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX)); // frameType, argv
    __ Add(Register(X0), sp, Immediate(QUADRUPLE_SLOT_SIZE));
    __ Blr(nativeFuncAddr);

    __ Mov(sp, Register(FP));
    __ Ldp(Register(X29), Register(X30), MemoryOperand(sp, DOUBLE_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Ldr(temp, MemoryOperand(sp, FRAME_SLOT_SIZE)); // argc
    __ Stp(temp, Register(Zero), MemoryOperand(sp, 0)); // argv, argc

    __ Ret();
}

// * uint64_t CallBuiltinConstructorStub(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...)
// * webkit_jscc calling convention call runtime_id's runtime function(c-abi)
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
//          |       thread             |                 |
//          |--------------------------|                 |
//          |       returnAddr         |    OptimizedBuiltinLeaveFrame
//  sp ---> |--------------------------|                 |
//          |       callsiteFp         |                 |
//          |--------------------------|                 |
//          |       frameType          |                 v
//          +--------------------------+ -----------------

void OptimizedCall::CallBuiltinConstructorStub(ExtendedAssembler *assembler, Register builtinStub, Register argv,
                                               Register glue, Register temp)
{
    Register sp(SP);

    __ Ldr(temp, MemoryOperand(sp, 0));
    __ Stp(glue, temp, MemoryOperand(sp, 0));   // argc, glue
    // returnAddr, callsiteFp
    __ Stp(Register(X29), Register(X30), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(temp, sp);
    __ Mov(Register(X29), temp); // rbp
    __ Mov(temp, Immediate(static_cast<int32_t>(FrameType::BUILTIN_CALL_LEAVE_FRAME)));
    __ Stp(Register(Zero), temp, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX)); // frameType, argv
    __ Add(argv, sp, Immediate(NONUPLE_SLOT_SIZE));
    __ Blr(builtinStub);

    __ Mov(sp, Register(FP));
    __ Ldp(Register(X29), Register(X30), MemoryOperand(sp, DOUBLE_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Ldr(temp, MemoryOperand(sp, FRAME_SLOT_SIZE)); // argc
    __ Stp(temp, Register(Zero), MemoryOperand(sp, 0)); // argv, argc

    __ Ret();
}

// * uint64_t JSCall(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, JSTaggedType new,
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
//               +--------------------------+
//               |       argv               |
//               |--------------------------|
//               |       argc               |
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
    __ Ldr(jsfunc, MemoryOperand(sp, DOUBLE_SLOT_SIZE)); // skip 2: argc, argv
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
    Label lCallConstructor;
    Label lCallBuiltinStub;
    Label lCallNativeCpp;
    Label lNotClass;

    __ Ldr(Register(X5), MemoryOperand(jsfunc, 0));
    __ Ldr(Register(X5), MemoryOperand(Register(X5), JSHClass::BIT_FIELD_OFFSET));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(actualArgC, MemoryOperand(sp, 0));
    __ Ldr(callField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Tbnz(callField, MethodLiteral::IsNativeBit::START_BIT, &callNativeMethod);
    if (!isNew) {
        __ Tbz(Register(X5), JSHClass::IsClassConstructorOrPrototypeBit::START_BIT, &lNotClass);
        __ Tbnz(Register(X5), JSHClass::ConstructorBit::START_BIT, &lCallConstructor);
    }
    __ Bind(&lNotClass);
    {
        Register argV(X5);
        // skip argc and argv
        __ Add(argV, sp, Immediate(kungfu::ArgumentAccessor::GetExtraArgsNum() * FRAME_SLOT_SIZE));
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
        __ Ldr(nativeFuncAddr, MemoryOperand(jsfunc, JSFunctionBase::CODE_ENTRY_OFFSET));
        CallBuiltinTrampoline(assembler);
    }

    __ Bind(&lCallBuiltinStub);
    {
        TempRegister1Scope scope1(assembler);
        Register builtinStub = __ TempRegister1();
        __ Ldr(Register(X5), MemoryOperand(method, Method::EXTRA_LITERAL_INFO_OFFSET));  // get extra literal
        __ And(Register(X5).W(), Register(X5).W(), LogicalImmediate::Create(0xff, RegWSize));
        if (!isNew) {
            __ Cmp(Register(X5).W(), Immediate(BUILTINS_STUB_ID(BUILTINS_CONSTRUCTOR_STUB_FIRST)));
            __ B(Condition::GE, &lCallNativeCpp);
        }
        __ Add(builtinStub, glue, Operand(Register(X5).W(), UXTW, FRAME_SLOT_SIZE_LOG2));
        __ Ldr(builtinStub, MemoryOperand(builtinStub, JSThread::GlueData::GetBuiltinsStubEntriesOffset(false)));

        __ Ldr(Register(X1), MemoryOperand(jsfunc, JSFunctionBase::CODE_ENTRY_OFFSET));
        __ Ldr(Register(X2), MemoryOperand(sp, DOUBLE_SLOT_SIZE));  // get jsfunc
        __ Ldr(Register(X3), MemoryOperand(sp, TRIPLE_SLOT_SIZE));  // get newtarget
        __ Ldr(Register(X4), MemoryOperand(sp, QUADRUPLE_SLOT_SIZE));  // get this
        __ Ldr(Register(X5), MemoryOperand(sp, 0));  // get number args
        __ Sub(Register(X5), Register(X5), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        if (!isNew) {
            Label lCall0;
            Label lCall1;
            Label lCall2;
            Label lCall3;
            Label lTailCall;
            Register fp(X29);
            __ Cmp(Register(X5), Immediate(0));
            __ B(Condition::EQ, &lCall0);
            __ Cmp(Register(X5), Immediate(1));
            __ B(Condition::EQ, &lCall1);
            __ Cmp(Register(X5), Immediate(2));  // 2: 2 args
            __ B(Condition::EQ, &lCall2);
            __ Cmp(Register(X5), Immediate(3));  // 3: 3 args
            __ B(Condition::EQ, &lCall3);

            __ Bind(&lCall0);
            {
                __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
                __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
                __ Str(Register(X7), MemoryOperand(sp, 0));  // reset arg2's position
                __ B(&lTailCall);
            }

            __ Bind(&lCall1);
            {
                __ Ldp(Register(X6), Register(X7), MemoryOperand(sp, QUINTUPLE_SLOT_SIZE));
                __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));  // reset x7
                __ Str(Register(X7), MemoryOperand(sp, 0));  // reset arg2's position
                __ B(&lTailCall);
            }

            __ Bind(&lCall2);
            {
                __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
                __ Str(Register(X7), MemoryOperand(sp, 0));  // reset arg2's position
                __ Ldp(Register(X6), Register(X7), MemoryOperand(sp, QUINTUPLE_SLOT_SIZE));
                __ B(&lTailCall);
            }

            __ Bind(&lCall3);
            __ Ldp(Register(X6), Register(X7), MemoryOperand(sp, QUINTUPLE_SLOT_SIZE));  // get arg0 arg1
            PushAsmBridgeFrame(assembler);
            {
                // push arg2 and call
                TempRegister2Scope scope2(assembler);
                Register arg2 = __ TempRegister2();
                __ Ldr(arg2, MemoryOperand(fp, NONUPLE_SLOT_SIZE));
                __ Stp(arg2, Register(X8), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
                __ Blr(builtinStub);
                __ Add(sp, sp, Immediate(DOUBLE_SLOT_SIZE));
            }
            PopAsmBridgeFrame(assembler);
            __ Ret();
            __ Bind(&lTailCall);
            {
                __ Br(builtinStub);
            }
        } else {
            Register argv(X6);
            TempRegister2Scope scope2(assembler);
            Register temp = __ TempRegister2();
            CallBuiltinConstructorStub(assembler, builtinStub, argv, glue, temp);
        }
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
        Register nativeFuncAddr(X4);
        __ Ldr(method, MemoryOperand(jsfunc, JSProxy::METHOD_OFFSET));
        __ Ldr(callField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
        __ Ldr(actualArgC, MemoryOperand(sp, 0));
        __ Ldr(nativeFuncAddr, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        CallBuiltinTrampoline(assembler);
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

// After the callee function of common aot call deopt, use this bridge to deal with this aot call.
// calling convention: webkit_jsc
// Input structure:
// %X0 - glue
// stack:
// +--------------------------+
// |       arg[N-1]           |
// +--------------------------+
// |       ...                |
// +--------------------------+
// |       arg[1]             |
// +--------------------------+
// |       arg[0]             |
// +--------------------------+
// |       this               |
// +--------------------------+
// |       new-target         |
// +--------------------------+
// |       call-target        |
// |--------------------------|
// |       argv               |
// |--------------------------|
// |       argc               |
// +--------------------------+ <---- sp
void OptimizedCall::AOTCallToAsmInterBridge(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(AOTCallToAsmInterBridge));
    Register sp(SP);
    // params of c++ calling convention
    Register glue(X0);
    Register jsfunc(X1);
    Register method(X2);
    Register callField(X3);
    Register actualArgC(X4);
    Register argV(X5);

    __ Ldr(jsfunc, MemoryOperand(sp, DOUBLE_SLOT_SIZE));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(callField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Ldr(actualArgC, MemoryOperand(sp, 0));
    // skip argc
    __ Add(argV, sp, Immediate(kungfu::ArgumentAccessor::GetExtraArgsNum() * FRAME_SLOT_SIZE));
    // asm interpreter argV = argv + 24
    __ Add(argV, argV, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE));
    __ Sub(actualArgC, actualArgC, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum()));
    OptimizedCallAsmInterpreter(assembler);
}

// After the callee function of fast aot call deopt, use this bridge to deal with this fast aot call.
// Notice: no argc and new-target params compared with not-fast aot call because these params are not needed
// by bytecode-analysis
// Intruduction: use expected argc as actual argc below for these reasons:
// 1) when expected argc == actual argc, pass.
// 2) when expected argc > actual argc, undefineds have been pushed in OptimizedFastCallAndPushArgv.
// 3) when expected argc < actual argc, redundant params are useless according to bytecode-analysis, just abandon them.
// calling convention: c++ calling convention
// Input structure:
// %X0 - glue
// %X1 - call-target
// %X2 - this
// %X3 - arg0
// %X4 - arg1
// %X5 - arg2
// %X6 - arg3
// %X7 - arg4
// stack:
// +--------------------------+
// |        arg[N-1]          |
// +--------------------------+
// |       ...                |
// +--------------------------+
// |       arg[5]             |
// +--------------------------+ <---- sp
void OptimizedCall::FastCallToAsmInterBridge(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(FastCallToAsmInterBridge));

    // Add a bridge frame to protect the stack map, because args will be put on the stack to construct argv on stack
    // and the AsmInterpBridgeFrame pushed below cannot protect the stack map anymore.
    PushAsmBridgeFrame(assembler);

    Register sp(SP);
    // Input
    Register glue(X0);
    Register jsfunc(X1);
    Register thisReg(X2);

    Register tempArgc = __ AvailableRegister1();
    {
        TempRegister2Scope scope2(assembler);
        Register tempMethod = __ TempRegister2();

        __ Ldr(tempMethod, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
        __ Ldr(tempArgc, MemoryOperand(tempMethod, Method::CALL_FIELD_OFFSET));
        __ Lsr(tempArgc, tempArgc, MethodLiteral::NumArgsBits::START_BIT);
        __ And(tempArgc, tempArgc,
            LogicalImmediate::Create(
                MethodLiteral::NumArgsBits::Mask() >> MethodLiteral::NumArgsBits::START_BIT, RegXSize));
    }
    {
        TempRegister1Scope scope1(assembler);
        Register startSp = __ TempRegister1();
        __ Mov(startSp, sp);

        Label lCall0;
        Label lCall1;
        Label lCall2;
        Label lCall3;
        Label lCall4;
        Label lCall5;
        Label lPushCommonRegs;

        __ Cmp(tempArgc, Immediate(0));
        __ B(Condition::EQ, &lCall0);
        __ Cmp(tempArgc, Immediate(1));
        __ B(Condition::EQ, &lCall1);
        __ Cmp(tempArgc, Immediate(2));  // 2: 2 args
        __ B(Condition::EQ, &lCall2);
        __ Cmp(tempArgc, Immediate(3));  // 3: 3 args
        __ B(Condition::EQ, &lCall3);
        __ Cmp(tempArgc, Immediate(4));  // 4: 4 args
        __ B(Condition::EQ, &lCall4);
        __ Cmp(tempArgc, Immediate(5));  // 5: 5 args
        __ B(Condition::EQ, &lCall5);
        // default: more than 5 args
        {
            TempRegister2Scope scope2(assembler);
            Register onStackArgs = __ TempRegister2();
            Register op1 = __ AvailableRegister2();
            Register op2 = __ AvailableRegister3();

            // skip bridge frame, return addr and a callee save
            __ Add(onStackArgs, sp, Immediate(QUADRUPLE_SLOT_SIZE));
            __ Sub(tempArgc, tempArgc, Immediate(5));  // 5: the first 5 args are not on stack
            Register arg4(X7);
            PushArgsWithArgvInPair(assembler, tempArgc, onStackArgs, arg4, op1, op2, &lCall4);
        }

        __ Bind(&lCall0);
        {
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall1);
        {
            __ Stp(Register(X3), Register(Zero), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall2);
        {
            __ Stp(Register(X3), Register(X4), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall3);
        {
            __ Stp(Register(X5), Register(Zero), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(Register(X3), Register(X4), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall4);
        {
            __ Stp(Register(X5), Register(X6), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(Register(X3), Register(X4), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall5);
        {
            __ Stp(Register(X7), Register(Zero), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(Register(X5), Register(X6), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(Register(X3), Register(X4), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lPushCommonRegs);
        {
            Register newTarget(X7);
            __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
            __ Stp(newTarget, thisReg, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(startSp, jsfunc, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            // fall through
        }
    }

    // params of c++ calling convention
    // glue: X0
    // jsfunc: X1
    Register method(X2);
    Register methodCallField(X3);
    Register argc(X4);
    Register argV(X5);
    // reload and prepare args for JSCallCommonEntry
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(methodCallField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Mov(argc, methodCallField);
    __ Lsr(argc, argc, MethodLiteral::NumArgsBits::START_BIT);
    __ And(argc, argc,
        LogicalImmediate::Create(
            MethodLiteral::NumArgsBits::Mask() >> MethodLiteral::NumArgsBits::START_BIT, RegXSize));
    __ Add(argV, sp, Immediate((kungfu::ArgumentAccessor::GetFixArgsNum() + 1) * FRAME_SLOT_SIZE));  // 1: skip startSp

    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    {
        PopAsmInterpBridgeFrame(assembler);
        TempRegister1Scope scope1(assembler);
        Register startSp = __ TempRegister1();
        __ Ldp(startSp, Register(Zero), MemoryOperand(sp, ExtendedAssembler::PAIR_SLOT_SIZE, POSTINDEX));
        __ Mov(sp, startSp);
        PopAsmBridgeFrame(assembler);
        __ Ret();
    }
    __ Bind(&target);
    {
        AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_FROM_AOT,
                                              FrameTransitionType::OTHER_TO_OTHER);
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
    __ Cmp(jstype, Immediate(static_cast<int>(JSType::JS_FUNCTION_LAST) -
        static_cast<int>(JSType::JS_FUNCTION_FIRST) + 1));
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

void OptimizedCall::JSBoundFunctionCallInternal(ExtendedAssembler *assembler, Register glue,
                                                Register actualArgC, Register jsfunc, int stubId)
{
    // construct frame
    PushOptimizedArgsConfigFrame(assembler);
    Register basefp(X29);
    Register fp = __ AvailableRegister1();

    Register argV(X5);
    __ Add(argV, basefp, Immediate(GetStackArgOffSetToFp(0))); // 0: first index id
    __ Ldr(actualArgC, MemoryOperand(argV, 0));

    Register boundLength(X2);
    Register realArgC(X7, W);
    Label copyBoundArgument;
    Label pushCallTarget;
    Label popArgs;
    Label slowCall;
    Label aotCall;
    Label notClass;
    // get bound arguments
    __ Ldr(boundLength, MemoryOperand(jsfunc, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
    //  get bound length
    __ Ldr(boundLength, MemoryOperand(boundLength, TaggedArray::LENGTH_OFFSET));
    __ Add(realArgC, boundLength.W(), actualArgC.W());
    __ Mov(Register(X19), realArgC);
    IncreaseStackForArguments(assembler, realArgC, fp, static_cast<int64_t>(CommonArgIdx::ACTUAL_ARGV));
    __ Sub(actualArgC.W(), actualArgC.W(), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Cmp(actualArgC.W(), Immediate(0));
    __ B(Condition::EQ, &copyBoundArgument);
    {
        TempRegister1Scope scope1(assembler);
        Register tmp = __ TempRegister1();
        const int64_t argoffsetSlot = static_cast<int64_t>(CommonArgIdx::FUNC) - 1;
        __ Add(argV, argV, Immediate((NUM_MANDATORY_JSFUNC_ARGS + argoffsetSlot) * FRAME_SLOT_SIZE));
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
    Register boundTarget(X7);
    Register newTarget(X6);
    __ Bind(&pushCallTarget);
    {
        Register thisObj(X4);
        __ Ldr(thisObj, MemoryOperand(jsfunc, JSBoundFunction::BOUND_THIS_OFFSET));
        __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        // 2 : 2 means pair
        __ Stp(newTarget, thisObj, MemoryOperand(fp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Ldr(boundTarget, MemoryOperand(jsfunc, JSBoundFunction::BOUND_TARGET_OFFSET));
        // 2 : 2 means pair
        __ Stp(argV, boundTarget, MemoryOperand(fp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Str(Register(X19), MemoryOperand(fp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    JSCallCheck(assembler, boundTarget, Register(X9), &slowCall, &slowCall);
    Register hclass = __ AvailableRegister2();
    __ Ldr(hclass, MemoryOperand(boundTarget, 0));
    __ Ldr(hclass, MemoryOperand(hclass, JSHClass::BIT_FIELD_OFFSET));
    __ Tbz(hclass, JSHClass::IsClassConstructorOrPrototypeBit::START_BIT, &notClass);
    __ Tbnz(hclass, JSHClass::ConstructorBit::START_BIT, &slowCall);
    __ Bind(&notClass);
    Register compiledCodeFlag(X9, W);
    __ Ldrh(compiledCodeFlag, MemoryOperand(boundTarget, JSFunctionBase::BIT_FIELD_OFFSET));
    __ Tbz(compiledCodeFlag, JSFunctionBase::IsCompiledCodeBit::START_BIT, &slowCall);
    __ Bind(&aotCall);
    {
        // output: glue:x0 argc:x1 calltarget:x2 argv:x3 this:x4 newtarget:x5
        __ Mov(Register(X1), Register(X19));
        __ Mov(Register(X2), boundTarget);
        __ Add(X3, fp, Immediate(5 * FRAME_SLOT_SIZE)); // 5: skip argc and argv func new this
        __ Mov(Register(X5), Register(X6));
        Register boundCallInternalId(X9);
        Register baseAddress(X8);
        Register codeAddress(X10);
        __ Mov(baseAddress, Immediate(JSThread::GlueData::GetCOStubEntriesOffset(false)));
        __ Mov(boundCallInternalId, Immediate(CommonStubCSigns::JsBoundCallInternal));
        __ Add(codeAddress, X0, baseAddress);
        __ Ldr(codeAddress, MemoryOperand(codeAddress, boundCallInternalId, UXTW, FRAME_SLOT_SIZE_LOG2));
        __ Blr(codeAddress);
        __ B(&popArgs);
    }
    __ Bind(&slowCall);
    {
        __ CallAssemblerStub(stubId, false);
        __ B(&popArgs);
    }

    __ Bind(&popArgs);
    PopJSFunctionArgs(assembler, Register(X19), Register(X19));
    PopOptimizedArgsConfigFrame(assembler);
    __ Ret();
}

// * uint64_t JSProxyCallInternalWithArgV(uintptr_t glue, JSTaggedType calltarget)
// * c++ calling convention call js function
// * Arguments:
//        %x0 - glue
//        %x1 - calltarget

void OptimizedCall::JSProxyCallInternalWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSProxyCallInternalWithArgV));
    Register jsfunc(X1);
    Register sp(SP);
    __ Str(jsfunc, MemoryOperand(sp, DOUBLE_SLOT_SIZE));
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
    __ Stp(Register(X30), runtimeId, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX)); // 2 : 2 means pair
    Register fp(X29);
    // construct leave frame
    Register frameType(X9);
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME_WITH_ARGV)));
    __ Stp(frameType, Register(X29), MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX)); // 2 : 2 means pair
    __ Add(Register(FP), sp, Immediate(FRAME_SLOT_SIZE));
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

     // load runtime trampoline address
    Register tmp(X9);
    Register rtfunc(X9);
    // 3 : 3 means 2 << 3 = 8
    __ Add(tmp, glue, Operand(runtimeId, LSL, 3));
    __ Ldr(rtfunc, MemoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Mov(X1, argc);
    __ Mov(X2, argv);
    __ Blr(rtfunc);
    __ Ldp(Register(Zero), Register(X29), MemoryOperand(sp, ExtendedAssembler::PAIR_SLOT_SIZE, POSTINDEX));
    __ Ldp(Register(X30), Register(Zero), MemoryOperand(sp, ExtendedAssembler::PAIR_SLOT_SIZE, POSTINDEX));
    __ Add(sp, sp, Immediate(2 * FRAME_SLOT_SIZE)); // 2 : 2 means pair
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

void OptimizedCall::PushJSFunctionEntryFrame(ExtendedAssembler *assembler, Register prevFp)
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
//               |       argn               |
//               |--------------------------|
//               |       argn - 1           |
//               |--------------------------|
//               |       .....              |
//               |--------------------------|
//               |       arg2               |
//               |--------------------------|
//               |       arg1               |
//      sp ----> |--------------------------| ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|  OptimizedJSFunctionFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------

void OptimizedCall::GenJSCallWithArgV(ExtendedAssembler *assembler, int id)
{
    Register sp(SP);
    Register glue(X0);
    Register actualNumArgs(X1);
    Register jsfunc(X2);
    Register newTarget(X3);
    Register thisObj(X4);
    Register argV(X5);
    Register currentSp = __ AvailableRegister1();
    Register callsiteSp = __ AvailableRegister2();
    Label pushCallThis;

    __ Mov(callsiteSp, sp);
    PushOptimizedUnfoldArgVFrame(assembler, callsiteSp);
    Register argC(X7);
    __ Add(actualNumArgs, actualNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Mov(argC, actualNumArgs);
    IncreaseStackForArguments(assembler, argC, currentSp, static_cast<int64_t>(CommonArgIdx::ACTUAL_ARGV));
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
    {
        TempRegister1Scope scope1(assembler);
        Register tmp = __ TempRegister1();
        __ Mov(tmp, currentSp);
        __ Str(tmp, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ Str(actualNumArgs, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ CallAssemblerStub(id, false);
    __ Ldr(actualNumArgs, MemoryOperand(sp, 0));
    PopJSFunctionArgs(assembler, actualNumArgs, actualNumArgs);
    PopOptimizedUnfoldArgVFrame(assembler);
    __ Ret();
}

// * uint64_t JSCallWithArgVAndPushArgv(uintptr_t glue, uint32_t argc, JSTaggedType calltarget,
//                          JSTaggedType new, JSTaggedType this, argV)
// * cc calling convention call js function()
// * arguments:
//              %x0 - glue
//              %x1 - argc
//              %x2 - call-target
//              %x3 - new-target
//              %x4  - this
//              %x5  - argv
void OptimizedCall::JSCallWithArgVAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallWithArgVAndPushArgv));
    GenJSCallWithArgV(assembler, RTSTUB_ID(OptimizedCallAndPushArgv));
}

void OptimizedCall::JSCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallWithArgV));
    GenJSCallWithArgV(assembler, RTSTUB_ID(CallOptimized));
}

void OptimizedCall::SuperCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SuperCallWithArgV));
    GenJSCallWithArgV(assembler, RTSTUB_ID(JSCallNew));
}

void OptimizedCall::CallOptimized(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallOptimized));
    Register sp(SP);
    Register jsfunc(X7);
    Register method(X6);
    Register codeAddr(X5);
    auto funcSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum();
    __ Ldr(jsfunc, MemoryOperand(sp, funcSlotOffset * FRAME_SLOT_SIZE));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(codeAddr, MemoryOperand(jsfunc, JSFunction::CODE_ENTRY_OFFSET));
    __ Br(codeAddr);
}

void OptimizedCall::DeoptEnterAsmInterpOrBaseline(ExtendedAssembler *assembler)
{
    // rdi
    Register glueRegister = __ GlueRegister();
    Register context(X2);
    Register opRegister(X9);
    Register outputCount(X10);
    Register frameStateBase(X11);
    Register currentSlotRegister(X12);
    Register sp(SP);
    Register depth(X20);
    Register tmpReg(X21);
    Label loopBegin;
    Label stackOverflow;
    Label pushArgv;

    __ PushFpAndLr();

    __ Ldr(depth, MemoryOperand(context, AsmStackContext::GetInlineDepthOffset(false)));
    __ Add(context, context, Immediate(AsmStackContext::GetSize(false)));
    __ Mov(Register(X23), Immediate(0));
    // update fp
    __ Mov(currentSlotRegister, sp);
    __ Bind(&loopBegin);
    __ Ldr(outputCount, MemoryOperand(context, 0));
    __ Add(frameStateBase, context, Immediate(FRAME_SLOT_SIZE));
    __ Cmp(Register(X23), Immediate(0));
    __ B(Condition::EQ, &pushArgv);
    __ Mov(tmpReg, currentSlotRegister);
    __ Add(tmpReg, tmpReg, Immediate(AsmInterpretedFrame::GetSize(false)));
    __ Add(Register(X9), frameStateBase, Immediate(AsmInterpretedFrame::GetBaseOffset(false)));
    __ Str(tmpReg, MemoryOperand(Register(X9), InterpretedFrameBase::GetPrevOffset(false)));
    __ Align16(currentSlotRegister);

    __ Bind(&pushArgv);
    __ Mov(tmpReg, outputCount);
    __ Str(currentSlotRegister, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetFpOffset(false)));
    PushArgsWithArgv(assembler, glueRegister, outputCount, frameStateBase, opRegister,
                     currentSlotRegister, nullptr, &stackOverflow);
    __ Add(context, context, Immediate(FRAME_SLOT_SIZE)); // skip outputCount
    __ Add(context, context, Operand(tmpReg, UXTW, FRAME_SLOT_SIZE_LOG2)); // skip args
    __ Add(Register(X23), Register(X23), Immediate(1));
    __ Cmp(depth, Register(X23));
    __ B(Condition::GE, &loopBegin);

    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);
    __ Ldr(callTargetRegister, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetFunctionOffset(false)));
    // get baseline code
    __ Ldr(opRegister, MemoryOperand(callTargetRegister, JSFunction::BASELINECODE_OFFSET));
    Label baselineCodeUndefined;
    __ Cmp(opRegister, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ B(Condition::EQ, &baselineCodeUndefined);

    // check is compiling
    __ Cmp(opRegister, Immediate(JSTaggedValue::VALUE_HOLE));
    __ B(Condition::EQ, &baselineCodeUndefined);
    {
        // x20 is free and callee save
        Register newSpRegister = X20;
        // get new sp
        __ Add(newSpRegister, currentSlotRegister, Immediate(AsmInterpretedFrame::GetSize(false)));
        __ Align16(currentSlotRegister);
        __ Mov(sp, currentSlotRegister);

        // save glue, callTarget
        __ Stp(glueRegister, callTargetRegister, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
        // callee save
        __ CalleeSave();

        // get bytecode pc
        Register bytecodePc = opRegister;
        __ Ldr(bytecodePc, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetPcOffset(false)));
        // get func
        Register func(X1);
        func = callTargetRegister;
        Register argC(X2);
        Register runtimeId(X3);
        __ Mov(argC, Immediate(2)); // 2: argc
        __ Mov(runtimeId, Immediate(RTSTUB_ID(GetNativePcOfstForBaseline)));
        // get native pc offset in baselinecode by bytecodePc in func
        __ Stp(func, bytecodePc, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
        __ Stp(runtimeId, argC, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
        __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);

        // 2: skip runtimeId argc func bytecodePc
        __ Add(sp, sp, Immediate(2 * DOUBLE_SLOT_SIZE));

        __ CalleeRestore();
        // restore glue, callTarget
        __ Ldp(X19, callTargetRegister, MemoryOperand(sp, DOUBLE_SLOT_SIZE, AddrMode::POSTINDEX));
        // restore method, fp
        __ Ldr(methodRegister, MemoryOperand(callTargetRegister, JSFunctionBase::METHOD_OFFSET));
        __ Mov(X21, methodRegister);
        __ Mov(Register(FP), newSpRegister);

        // update pc
        const int64_t pcOffsetFromSp = -24; // -24: 3 slots, frameType, prevFrame, pc
        __ Mov(opRegister, Immediate(BASELINEJIT_PC_FLAG));
        __ Stur(opRegister, MemoryOperand(Register(FP), pcOffsetFromSp));

        // jmp to baselinecode
        __ Br(X0);
    }

    __ Bind(&baselineCodeUndefined);
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
    Register frameType(X3);
    Register glueReg(X0);

    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::ASM_BRIDGE_FRAME)));
    __ Stp(glueReg, frameType, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
    __ CalleeSave();

    Register deoptType(X1);
    Register depth(X2);
    Register argC(X3);
    Register runtimeId(X4);
    __ Stp(deoptType, depth, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(argC, Immediate(2)); // 2: argc
    __ Mov(runtimeId, Immediate(RTSTUB_ID(DeoptHandler)));
    __ Stp(runtimeId, argC, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
    __ Add(sp, sp, Immediate(2 * DOUBLE_SLOT_SIZE)); // 2: skip runtimeId, argc, depth, shiftLen

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
    DeoptEnterAsmInterpOrBaseline(assembler);

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
