/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64_constants.h"
#include "ecmascript/compiler/trampoline/aarch64/common_call.h"

#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/js_tagged_value.h"
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
    Register glue = x0;
    Register tmp = x19;
    Register argC = x1;
    Register argV = x2;

    __ BindAssemblerStub(RTSTUB_ID(CallRuntime));
    __ PushFpAndLr();

    Register frameType = x2;
    // construct Leave Frame and callee save
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME)));
    // 2 : 2 means pairs
    __ Stp(tmp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(2 * FRAME_SLOT_SIZE));  // 2 : 2 means pairs
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

    // load runtime trampoline address
    Register rtfunc = x19;
    __ Ldr(tmp, MemoryOperand(fp, GetStackArgOffSetToFp(0)));  // 0: the first arg id
    // 3 : 3 means 2 << 3 = 8
    __ Add(tmp, glue, Operand(tmp, LSL, 3));
    __ Ldr(rtfunc, MemoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Ldr(argC, MemoryOperand(fp, GetStackArgOffSetToFp(1)));  // 1: the second arg id
    __ Add(argV, fp, Immediate(GetStackArgOffSetToFp(2)));  // 2: the third arg id
#ifdef ENABLE_CMC_IR_FIX_REGISTER
    __ Mov(x28, glue); // move glue to a callee-save register
#endif
    __ Blr(rtfunc);
    __ UpdateGlueAndReadBarrier();
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
    __ Mov(currentSp, sp);
    if (numExtraArgs > 0) {
        // add extra aguments, numArgs
        __ Add(argc, argc, Immediate(numExtraArgs));
    }
    __ Sub(currentSp, currentSp, Operand(argc, UXTW, FRAME_SLOT_SIZE_LOG2));
    Label aligned;
    __ Tst(currentSp, LogicalImmediate::Create(0xf, X_REG_SIZE));  // 0xf: 0x1111
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
    Register glueReg = x0;
    Register argV = x2;
    Register prevFpReg = x3;
    Register needPushArgv = x4;
    Register tmpArgV = x7;
    Label lJSCallWithArgVAndPushArgv;
    Label lPopFrame;

    PushJSFunctionEntryFrame (assembler, prevFpReg);
    __ UpdateGlueAndReadBarrier(glueReg);
    __ Mov(x6, needPushArgv);
    __ Mov(tmpArgV, argV);
    __ Mov(x20, glueReg);
    __ Ldr(x2, MemoryOperand(tmpArgV, 0));
    __ Ldr(x3, MemoryOperand(tmpArgV, FRAME_SLOT_SIZE));
    __ Ldr(x4, MemoryOperand(tmpArgV, DOUBLE_SLOT_SIZE));
    __ Add(tmpArgV, tmpArgV, Immediate(TRIPLE_SLOT_SIZE));
    __ Mov(x5, tmpArgV);
    __ Cmp(x6, Immediate(1));
    __ B(Condition::EQ, &lJSCallWithArgVAndPushArgv);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgV), false);
    __ B(&lPopFrame);

    __ Bind(&lJSCallWithArgVAndPushArgv);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgVAndPushArgv), false);
    __ Bind(&lPopFrame);
    __ Mov(x2, x20);
    PopJSFunctionEntryFrame(assembler, x2);
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
    Register jsfunc = x7;
    Register method = x6;
    Register expectedNumArgs = x1;
    Register actualNumArgs = x2;
    Register codeAddr = x3;
    Register argV = x4;

    auto funcSlotOffSet = kungfu::ArgumentAccessor::GetExtraArgsNum();
    __ Ldr(jsfunc, MemoryOperand(sp, funcSlotOffSet * FRAME_SLOT_SIZE));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(codeAddr, MemoryOperand(jsfunc, JSFunction::CODE_ENTRY_OFFSET));
    __ Ldr(expectedNumArgs, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Lsr(expectedNumArgs, expectedNumArgs, Method::NumArgsBits::START_BIT);
    __ And(expectedNumArgs, expectedNumArgs,
        LogicalImmediate::Create(
            Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT, X_REG_SIZE));
    __ Add(expectedNumArgs, expectedNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));

    __ Add(argV, sp, Immediate(funcSlotOffSet * FRAME_SLOT_SIZE));  // skip numArgs and argv
    __ Ldr(actualNumArgs, MemoryOperand(sp, 0));

    Register glue = x0;
    Register currentSp = x5;
    Label copyArguments;
    Label invokeCompiledJSFunction;

    // construct frame
    PushOptimizedArgsConfigFrame(assembler);
    Register argC = x7;
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
        __ Mov(x19, expectedNumArgs);
        __ Str(currentSp, MemoryOperand(sp, FRAME_SLOT_SIZE));
        __ Str(actualNumArgs, MemoryOperand(sp, 0)); // argv, argc
        __ Blr(codeAddr);
    }

    // pop argV argC
    // 3 : 3 means argC * 8
    __ Ldr(actualNumArgs, MemoryOperand(sp, 0));
    PopJSFunctionArgs(assembler, x19, actualNumArgs);
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
    Register glue = x0;
    Register nativeFuncAddr = x4;
    Register temp = x1;

    // remove argv
    __ Ldr(temp, MemoryOperand(sp, 0));
    __ Stp(glue, temp, MemoryOperand(sp, 0));   // argc, glue
    // returnAddr, callsiteFp
    __ Stp(x29, x30, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(temp, sp);
    __ Str(temp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false))); // rbp
    __ Mov(x29, temp); // rbp
    __ Mov(temp, Immediate(static_cast<int32_t>(FrameType::BUILTIN_CALL_LEAVE_FRAME)));
    __ Stp(xzr, temp, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX)); // frameType, argv
#ifdef ENABLE_CMC_IR_FIX_REGISTER
    __ Mov(x28, glue); // move glue to a callee-save register
#endif
    __ Add(x0, sp, Immediate(QUADRUPLE_SLOT_SIZE));
    __ Blr(nativeFuncAddr);
    __ UpdateGlueAndReadBarrier();

    __ Mov(sp, fp);
    __ Ldp(x29, x30, MemoryOperand(sp, DOUBLE_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Ldr(temp, MemoryOperand(sp, FRAME_SLOT_SIZE)); // argc
    __ Stp(temp, xzr, MemoryOperand(sp, 0)); // argv, argc

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
    __ Ldr(temp, MemoryOperand(sp, 0));
    __ Stp(glue, temp, MemoryOperand(sp, 0));   // argc, glue
    // returnAddr, callsiteFp
    __ Stp(x29, x30, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(temp, sp);
    __ Mov(x29, temp); // rbp
    __ Mov(temp, Immediate(static_cast<int32_t>(FrameType::BUILTIN_CALL_LEAVE_FRAME)));
    __ Stp(xzr, temp, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX)); // frameType, argv
    __ Add(argv, sp, Immediate(NONUPLE_SLOT_SIZE));
    __ Blr(builtinStub);

    __ Mov(sp, fp);
    __ Ldp(x29, x30, MemoryOperand(sp, DOUBLE_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Ldr(temp, MemoryOperand(sp, FRAME_SLOT_SIZE)); // argc
    __ Stp(temp, xzr, MemoryOperand(sp, 0)); // argv, argc

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
    Register jsfunc = x1;
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
    Register glue = x0;
    Register taggedValue = x2;
    Label nonCallable;
    Label notJSFunction;
    JSCallCheck(assembler, jsfunc, taggedValue, &nonCallable, &notJSFunction);

    Register method = x2;
    Register callField = x3;
    Register actualArgC = x4;
    Label callNativeMethod;
    Label lCallConstructor;
    Label lCallBuiltinStub;
    Label lCallNativeCpp;
    Label lNotClass;

    __ Ldr(x5, MemoryOperand(jsfunc, JSFunction::HCLASS_OFFSET));
    __ And(x5, x5, LogicalImmediate::Create(TaggedObject::GC_STATE_MASK, X_REG_SIZE));
    __ Ldr(x5, MemoryOperand(x5, JSHClass::BIT_FIELD_OFFSET));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(actualArgC, MemoryOperand(sp, 0));
    __ Ldr(callField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Tbnz(callField, Method::IsNativeBit::START_BIT, &callNativeMethod);
    if (!isNew) {
        __ Tbz(x5, JSHClass::IsClassConstructorOrPrototypeBit::START_BIT, &lNotClass);
        __ Tbnz(x5, JSHClass::ConstructorBit::START_BIT, &lCallConstructor);
    }
    __ Bind(&lNotClass);
    {
        Register argV = x5;
        // skip argc and argv
        __ Add(argV, sp, Immediate(kungfu::ArgumentAccessor::GetExtraArgsNum() * FRAME_SLOT_SIZE));
        // asm interpreter argV = argv + 24
        __ Add(argV, argV, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE));
        __ Sub(actualArgC, actualArgC, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum()));
        OptimizedCallAsmInterpreter(assembler);
    }

    __ Bind(&callNativeMethod);
    {
        Register nativeFuncAddr = x4;
        if (!isNew) {
            __ Tbz(callField, Method::IsFastBuiltinBit::START_BIT, &lCallNativeCpp);
            // 3 : 3 means call0 call1 call2 call3
            __ Cmp(actualArgC, Immediate(kungfu::ArgumentAccessor::GetFixArgsNum() + 3));
            __ B(Condition::LE, &lCallBuiltinStub);
        } else {
            __ Tbnz(callField, Method::IsFastBuiltinBit::START_BIT, &lCallBuiltinStub);
        }
        __ Bind(&lCallNativeCpp);
        __ Ldr(nativeFuncAddr, MemoryOperand(jsfunc, JSFunctionBase::CODE_ENTRY_OFFSET));
        CallBuiltinTrampoline(assembler);
    }

    __ Bind(&lCallBuiltinStub);
    {
        TempRegister1Scope scope1(assembler);
        Register builtinStub = __ TempRegister1();
        __ Ldr(x5, MemoryOperand(method, Method::EXTRA_LITERAL_INFO_OFFSET));  // get extra literal
        __ Lsr(x5.W(), x5.W(), Method::BuiltinIdBits::START_BIT);
        __ And(x5.W(), x5.W(),
               LogicalImmediate::Create((1LU << Method::BuiltinIdBits::SIZE) - 1, W_REG_SIZE));
        if (!isNew) {
            __ Cmp(x5.W(), Immediate(BUILTINS_STUB_ID(BUILTINS_CONSTRUCTOR_STUB_FIRST)));
            __ B(Condition::GE, &lCallNativeCpp);
        }
        __ Add(builtinStub, glue, Operand(x5.W(), UXTW, FRAME_SLOT_SIZE_LOG2));
        __ Ldr(builtinStub, MemoryOperand(builtinStub, JSThread::GlueData::GetBuiltinsStubEntriesOffset(false)));

        __ Ldr(x1, MemoryOperand(jsfunc, JSFunctionBase::CODE_ENTRY_OFFSET));
        __ Ldr(x2, MemoryOperand(sp, DOUBLE_SLOT_SIZE));  // get jsfunc
        __ Ldr(x3, MemoryOperand(sp, TRIPLE_SLOT_SIZE));  // get newtarget
        __ Ldr(x4, MemoryOperand(sp, QUADRUPLE_SLOT_SIZE));  // get this
        __ Ldr(x5, MemoryOperand(sp, 0));  // get number args
        __ Sub(x5, x5, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        if (!isNew) {
            Label lCall0;
            Label lCall1;
            Label lCall2;
            Label lCall3;
            Label lTailCall;
            __ Cmp(x5, Immediate(0));
            __ B(Condition::EQ, &lCall0);
            __ Cmp(x5, Immediate(1));
            __ B(Condition::EQ, &lCall1);
            __ Cmp(x5, Immediate(2));  // 2: 2 args
            __ B(Condition::EQ, &lCall2);
            __ Cmp(x5, Immediate(3));  // 3: 3 args
            __ B(Condition::EQ, &lCall3);

            __ Bind(&lCall0);
            {
                __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
                __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
                __ Str(x7, MemoryOperand(sp, 0));  // reset arg2's position
                __ B(&lTailCall);
            }

            __ Bind(&lCall1);
            {
                __ Ldp(x6, x7, MemoryOperand(sp, QUINTUPLE_SLOT_SIZE));
                __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));  // reset x7
                __ Str(x7, MemoryOperand(sp, 0));  // reset arg2's position
                __ B(&lTailCall);
            }

            __ Bind(&lCall2);
            {
                __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
                __ Str(x7, MemoryOperand(sp, 0));  // reset arg2's position
                __ Ldp(x6, x7, MemoryOperand(sp, QUINTUPLE_SLOT_SIZE));
                __ B(&lTailCall);
            }

            __ Bind(&lCall3);
            __ Ldp(x6, x7, MemoryOperand(sp, QUINTUPLE_SLOT_SIZE));  // get arg0 arg1
            PushAsmBridgeFrame(assembler);
            {
                // push arg2 and call
                TempRegister2Scope scope2(assembler);
                Register arg2 = __ TempRegister2();
                __ Ldr(arg2, MemoryOperand(fp, NONUPLE_SLOT_SIZE));
                __ Stp(arg2, x8, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
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
            Register argv = x6;
            TempRegister2Scope scope2(assembler);
            Register temp = __ TempRegister2();
            CallBuiltinConstructorStub(assembler, builtinStub, argv, glue, temp);
        }
    }

    Label jsBoundFunction;
    Label jsProxy;
    __ Bind(&notJSFunction);
    {
        Register bitfield = w2;
        Register jstype2 = w5;
        __ And(jstype2, bitfield, LogicalImmediate::Create(0xff, W_REG_SIZE));
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
        Register nativeFuncAddr = x4;
        __ Ldr(method, MemoryOperand(jsfunc, JSProxy::METHOD_OFFSET));
        __ Ldr(callField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
        __ Ldr(actualArgC, MemoryOperand(sp, 0));
        __ Ldr(nativeFuncAddr, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        CallBuiltinTrampoline(assembler);
    }
    __ Bind(&nonCallable);
    {
        ThrowNonCallableInternal(assembler, sp, jsfunc);
    }
    __ Bind(&lCallConstructor);
    {
        Register frameType = x6;
        __ PushFpAndLr();
        __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
        // 2 : 2 means pair
        __ Stp(xzr, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
        Register argC = x5;
        Register runtimeId = x6;
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
    // params of c++ calling convention
    Register glue = x0;
    Register jsfunc = x1;
    Register method = x2;
    Register callField = x3;
    Register actualArgC = x4;
    Register argV = x5;

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

    // Input
    Register glue = x0;
    Register jsfunc = x1;
    Register thisReg = x2;

    Register tempArgc = __ AvailableRegister1();
    {
        TempRegister2Scope scope2(assembler);
        Register tempMethod = __ TempRegister2();

        __ Ldr(tempMethod, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
        __ Ldr(tempArgc, MemoryOperand(tempMethod, Method::CALL_FIELD_OFFSET));
        __ Lsr(tempArgc, tempArgc, Method::NumArgsBits::START_BIT);
        __ And(tempArgc, tempArgc,
            LogicalImmediate::Create(
                Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT, X_REG_SIZE));
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
            Register arg4 = x7;
            PushArgsWithArgvInPair(assembler, tempArgc, onStackArgs, arg4, op1, op2, &lCall4);
        }

        __ Bind(&lCall0);
        {
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall1);
        {
            __ Stp(x3, xzr, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall2);
        {
            __ Stp(x3, x4, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall3);
        {
            __ Stp(x5, xzr, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(x3, x4, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall4);
        {
            __ Stp(x5, x6, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(x3, x4, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lCall5);
        {
            __ Stp(x7, xzr, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(x5, x6, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(x3, x4, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ B(&lPushCommonRegs);
        }

        __ Bind(&lPushCommonRegs);
        {
            Register newTarget = x7;
            __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
            __ Stp(newTarget, thisReg, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            __ Stp(startSp, jsfunc, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
            // fall through
        }
    }

    // params of c++ calling convention
    // glue: x0
    // jsfunc: x1
    Register method = x2;
    Register methodCallField = x3;
    Register argc = x4;
    Register argV = x5;
    // reload and prepare args for JSCallCommonEntry
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(methodCallField, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Mov(argc, methodCallField);
    __ Lsr(argc, argc, Method::NumArgsBits::START_BIT);
    __ And(argc, argc,
        LogicalImmediate::Create(
            Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT, X_REG_SIZE));
    __ Add(argV, sp, Immediate((kungfu::ArgumentAccessor::GetFixArgsNum() + 1) * FRAME_SLOT_SIZE));  // 1: skip startSp

    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    {
        PopAsmInterpBridgeFrame(assembler);
        TempRegister1Scope scope1(assembler);
        Register startSp = __ TempRegister1();
        __ Ldp(startSp, xzr, MemoryOperand(sp, ExtendedAssembler::PAIR_SLOT_SIZE, POSTINDEX));
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
    __ Mov(taggedValue, Immediate(JSTaggedValue::TAG_MARK));
    __ Cmp(jsfunc, taggedValue);
    __ B(Condition::HS, nonCallable);
    __ Cbz(jsfunc, nonCallable);
    __ Mov(taggedValue, Immediate(JSTaggedValue::TAG_SPECIAL));
    __ And(taggedValue, jsfunc, taggedValue);
    __ Cbnz(taggedValue, nonCallable);

    Register jshclass = x2;
    Register glue = x0;
    __ Ldr(jshclass, MemoryOperand(jsfunc, JSFunction::HCLASS_OFFSET));
    __ And(jshclass, jshclass, LogicalImmediate::Create(TaggedObject::GC_STATE_MASK, X_REG_SIZE));
    Register bitfield = x2;
    __ Ldr(bitfield, MemoryOperand(jshclass, JSHClass::BIT_FIELD_OFFSET));
    __ Tbz(bitfield, JSHClass::CallableBit::START_BIT, nonCallable);

    Register jstype = w3;
    __ And(jstype, bitfield, LogicalImmediate::Create(0xFF, W_REG_SIZE));
    // 4 : 4 means JSType::JS_FUNCTION_FIRST
    __ Sub(jstype, jstype, Immediate(static_cast<int>(JSType::JS_FUNCTION_FIRST)));
    // 9 : 9 means JSType::JS_FUNCTION_LAST - JSType::JS_FUNCTION_FIRST + 1
    __ Cmp(jstype, Immediate(static_cast<int>(JSType::JS_FUNCTION_LAST) -
        static_cast<int>(JSType::JS_FUNCTION_FIRST) + 1));
    __ B(Condition::HS, notJSFunction);
}

void OptimizedCall::ThrowNonCallableInternal(ExtendedAssembler *assembler, Register sp, Register jsfunc)
{
    Register frameType = x6;
    __ PushFpAndLr();
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
    // 2 : 2 means pair
    __ Stp(jsfunc, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
    Register argC = x5;
    Register runtimeId = x6;
    __ Mov(argC, Immediate(1));
    __ Mov(runtimeId, Immediate(RTSTUB_ID(ThrowNotCallableException)));
    // 2 : 2 means pair
    __ Stp(runtimeId, argC, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
    __ Mov(x0, Immediate(JSTaggedValue::VALUE_EXCEPTION));
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
    Register fpReg = __ AvailableRegister1();

    Register argV = x5;
    __ Add(argV, fp, Immediate(GetStackArgOffSetToFp(0))); // 0: first index id
    __ Ldr(actualArgC, MemoryOperand(argV, 0));

    Register boundLength = x2;
    Register realArgC = w7;
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
    __ Mov(x19, realArgC);
    IncreaseStackForArguments(assembler, realArgC, fpReg, static_cast<int64_t>(CommonArgIdx::ACTUAL_ARGV));
    __ Sub(actualArgC.W(), actualArgC.W(), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Cmp(actualArgC.W(), Immediate(0));
    __ B(Condition::EQ, &copyBoundArgument);
    {
        TempRegister1Scope scope1(assembler);
        Register tmp = __ TempRegister1();
        const int64_t argoffsetSlot = static_cast<int64_t>(CommonArgIdx::FUNC) - 1;
        __ Add(argV, argV, Immediate((NUM_MANDATORY_JSFUNC_ARGS + argoffsetSlot) * FRAME_SLOT_SIZE));
        PushArgsWithArgv(assembler, glue, actualArgC, argV, tmp, fpReg, nullptr, nullptr);
    }
    __ Bind(&copyBoundArgument);
    {
        Register boundArgs = x4;
        __ Ldr(boundArgs, MemoryOperand(jsfunc, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
        __ Add(boundArgs, boundArgs, Immediate(TaggedArray::DATA_OFFSET));
        __ Cmp(boundLength.W(), Immediate(0));
        __ B(Condition::EQ, &pushCallTarget);
        {
            TempRegister1Scope scope1(assembler);
            Register tmp = __ TempRegister1();
            PushArgsWithArgv(assembler, glue, boundLength, boundArgs, tmp, fpReg, nullptr, nullptr);
        }
    }
    Register boundTarget = x7;
    Register newTarget = x6;
    __ Bind(&pushCallTarget);
    {
        Register thisObj = x4;
        __ Ldr(thisObj, MemoryOperand(jsfunc, JSBoundFunction::BOUND_THIS_OFFSET));
        __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        // 2 : 2 means pair
        __ Stp(newTarget, thisObj, MemoryOperand(fpReg, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Ldr(boundTarget, MemoryOperand(jsfunc, JSBoundFunction::BOUND_TARGET_OFFSET));
        // 2 : 2 means pair
        __ Stp(argV, boundTarget, MemoryOperand(fpReg, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Str(x19, MemoryOperand(fpReg, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    JSCallCheck(assembler, boundTarget, x9, &slowCall, &slowCall);
    Register hclass = __ AvailableRegister2();
    __ Ldr(hclass, MemoryOperand(boundTarget, JSFunction::HCLASS_OFFSET));
    __ And(hclass, hclass, LogicalImmediate::Create(TaggedObject::GC_STATE_MASK, X_REG_SIZE));
    __ Ldr(hclass, MemoryOperand(hclass, JSHClass::BIT_FIELD_OFFSET));
    __ Tbz(hclass, JSHClass::IsClassConstructorOrPrototypeBit::START_BIT, &notClass);
    __ Tbnz(hclass, JSHClass::ConstructorBit::START_BIT, &slowCall);
    __ Bind(&notClass);
    Register compiledCodeFlag = w9;
    __ Ldrh(compiledCodeFlag, MemoryOperand(boundTarget, JSFunctionBase::BIT_FIELD_OFFSET));
    __ Tbz(compiledCodeFlag, JSFunctionBase::IsCompiledCodeBit::START_BIT, &slowCall);
    __ Bind(&aotCall);
    {
        // output: glue:x0 argc:x1 calltarget:x2 argv:x3 this:x4 newtarget:x5
        __ Mov(x1, x19);
        __ Mov(x2, boundTarget);
        __ Add(x3, fpReg, Immediate(5 * FRAME_SLOT_SIZE)); // 5: skip argc and argv func new this
        __ Mov(x5, x6);
        Register boundCallInternalId = x9;
        Register baseAddress = x8;
        Register codeAddress = x10;
        __ Mov(baseAddress, Immediate(JSThread::GlueData::GetCOStubEntriesOffset(false)));
        __ Mov(boundCallInternalId, Immediate(CommonStubCSigns::JsBoundCallInternal));
        __ Add(codeAddress, x0, baseAddress);
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
    PopJSFunctionArgs(assembler, x19, x19);
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
    Register jsfunc = x1;
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
    Register glue = x0;
    Register runtimeId = x1;
    Register argc = x2;
    Register argv = x3;
    // 2 : 2 means pair
    __ Stp(argc, argv, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Stp(x30, runtimeId, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX)); // 2 : 2 means pair
    // construct leave frame
    Register frameType = x9;
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME_WITH_ARGV)));
    __ Stp(frameType, x29, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX)); // 2 : 2 means pair
    __ Add(fp, sp, Immediate(FRAME_SLOT_SIZE));
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

     // load runtime trampoline address
    Register tmp = x9;
    Register rtfunc = x9;
    // 3 : 3 means 2 << 3 = 8
    __ Add(tmp, glue, Operand(runtimeId, LSL, 3));
    __ Ldr(rtfunc, MemoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Mov(x1, argc);
    __ Mov(x2, argv);
#ifdef ENABLE_CMC_IR_FIX_REGISTER
    __ Mov(x28, glue); // move glue to a callee-save register
#endif
    __ Blr(rtfunc);
    __ UpdateGlueAndReadBarrier();
    __ Ldp(xzr, x29, MemoryOperand(sp, ExtendedAssembler::PAIR_SLOT_SIZE, POSTINDEX));
    __ Ldp(x30, xzr, MemoryOperand(sp, ExtendedAssembler::PAIR_SLOT_SIZE, POSTINDEX));
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
    Register fpReg = x6;
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
    __ Mov(fpReg, sp);
    __ Tst(fpReg, LogicalImmediate::Create(0xf, X_REG_SIZE));  // 0xf: 0x1111
    __ B(Condition::EQ, &aligned);
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ Bind(&aligned);
}

void OptimizedCall::PushJSFunctionEntryFrame(ExtendedAssembler *assembler, Register prevFp)
{
    TempRegister2Scope temp2Scope(assembler);
    __ PushFpAndLr();
    Register frameType = __ TempRegister2();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_ENTRY_FRAME)));
    // 2 : 2 means pairs
    __ Stp(prevFp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
    __ CalleeSave();
}

void OptimizedCall::PopJSFunctionEntryFrame(ExtendedAssembler *assembler, Register glue)
{
    Register prevFp = x1;
    __ CalleeRestore();

    // 2: prevFp and frameType
    __ Ldp(prevFp, xzr, MemoryOperand(sp, FRAME_SLOT_SIZE * 2, AddrMode::POSTINDEX));
    // restore return address
    __ RestoreFpAndLr();
    __ Str(prevFp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
}

void OptimizedCall::PushOptimizedArgsConfigFrame(ExtendedAssembler *assembler)
{
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    __ PushFpAndLr();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
    // 2 : 2 means pairs. X19 means calleesave and 16bytes align
    __ Stp(x19, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
}

void OptimizedCall::PopOptimizedArgsConfigFrame(ExtendedAssembler *assembler)
{
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    // 2 : 2 means pop call site sp and type
    __ Ldp(x19, frameType, MemoryOperand(sp, FRAME_SLOT_SIZE * 2, AddrMode::POSTINDEX));
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
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    __ PushFpAndLr();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME)));
    // 2 : 2 means pairs
    __ Stp(callSiteSp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
}

void OptimizedCall::PopOptimizedUnfoldArgVFrame(ExtendedAssembler *assembler)
{
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
    Register glue = x0;
    Register actualNumArgs = x1;
    Register jsfunc = x2;
    Register newTarget = x3;
    Register thisObj = x4;
    Register argV = x5;
    Register currentSp = __ AvailableRegister1();
    Register callsiteSp = __ AvailableRegister2();
    Label pushCallThis;

    __ Mov(callsiteSp, sp);
    PushOptimizedUnfoldArgVFrame(assembler, callsiteSp);
    Register argC = x7;
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
    Register jsfunc = x7;
    Register method = x6;
    Register codeAddr = x5;
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
    Register context = x2;
    Register opRegister = x9;
    Register outputCount = x10;
    Register frameStateBase = x11;
    Register currentSlotRegister = x12;
    Register depth = x20;
    Register tmpReg = x21;
    Register hasExceptionRegister = x25;
    Label loopBegin;
    Label stackOverflow;
    Label pushArgv;
    Label gotoExceptionHandler;

    __ PushFpAndLr();
    __ Ldr(hasExceptionRegister, MemoryOperand(context, AsmStackContext::GetHasExceptionOffset(false)));
    __ Ldr(depth, MemoryOperand(context, AsmStackContext::GetInlineDepthOffset(false)));
    __ Add(context, context, Immediate(AsmStackContext::GetSize(false)));
    __ Mov(x23, Immediate(0));
    // update fp
    __ Mov(currentSlotRegister, sp);
    __ Bind(&loopBegin);
    __ Ldr(outputCount, MemoryOperand(context, 0));
    __ Add(frameStateBase, context, Immediate(FRAME_SLOT_SIZE));
    __ Cmp(x23, Immediate(0));
    __ B(Condition::EQ, &pushArgv);
    __ Mov(tmpReg, currentSlotRegister);
    __ Add(tmpReg, tmpReg, Immediate(AsmInterpretedFrame::GetSize(false)));
    __ Add(x9, frameStateBase, Immediate(AsmInterpretedFrame::GetBaseOffset(false)));
    __ Str(tmpReg, MemoryOperand(x9, InterpretedFrameBase::GetPrevOffset(false)));
    __ Align16(currentSlotRegister);

    __ Bind(&pushArgv);
    __ Mov(tmpReg, outputCount);
    __ Str(currentSlotRegister, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetFpOffset(false)));
    PushArgsWithArgv(assembler, glueRegister, outputCount, frameStateBase, opRegister,
                     currentSlotRegister, nullptr, &stackOverflow);
    __ Add(context, context, Immediate(FRAME_SLOT_SIZE)); // skip outputCount
    __ Add(context, context, Operand(tmpReg, UXTW, FRAME_SLOT_SIZE_LOG2)); // skip args
    __ Add(x23, x23, Immediate(1));
    __ Cmp(depth, x23);
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
        Register newSpRegister = x20;
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
        Register func = x1;
        func = callTargetRegister;
        Register argC = x2;
        Register runtimeId = x3;
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
        __ Ldp(x19, callTargetRegister, MemoryOperand(sp, DOUBLE_SLOT_SIZE, AddrMode::POSTINDEX));
        // restore method, fp
        __ Ldr(methodRegister, MemoryOperand(callTargetRegister, JSFunctionBase::METHOD_OFFSET));
        __ Mov(x21, methodRegister);
        __ Mov(fp, newSpRegister);

        // update pc
        const int64_t pcOffsetFromSp = -24; // -24: 3 slots, frameType, prevFrame, pc
        __ Mov(opRegister, Immediate(BASELINEJIT_PC_FLAG));
        __ Stur(opRegister, MemoryOperand(fp, pcOffsetFromSp));

        // jmp to baselinecode
        __ Br(x0);
    }

    __ Bind(&baselineCodeUndefined);
    {
        // X19, fp, x20, x21,      x22,     x23,  x24
        // glue sp   pc  constpool  profile  acc   hotness
        __ Ldr(callTargetRegister, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetFunctionOffset(false)));
        __ Ldr(x20, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetPcOffset(false)));
        __ Ldr(x23, MemoryOperand(frameStateBase, AsmInterpretedFrame::GetAccOffset(false)));
        __ Ldr(methodRegister, MemoryOperand(callTargetRegister, JSFunctionBase::METHOD_OFFSET));

        __ Add(opRegister, currentSlotRegister, Immediate(AsmInterpretedFrame::GetSize(false)));

        __ Align16(currentSlotRegister);
        __ Mov(sp, currentSlotRegister);

        __ Cmp(hasExceptionRegister, Immediate(0));
        __ B(Condition::NE, &gotoExceptionHandler);
        AsmInterpreterCall::DispatchCall(assembler, x20, opRegister, x23, false);
        __ Bind(&gotoExceptionHandler);
        AsmInterpreterCall::DispatchCall(assembler, x20, opRegister, x23, true);
    }
    __ Bind(&stackOverflow);
    {
        Register temp = x1;
        AsmInterpreterCall::ThrowStackOverflowExceptionAndReturnToAsmInterpBridgeFrame(
            assembler, glueRegister, sp, temp);
    }
}

void OptimizedCall::DeoptPushAsmInterpBridgeFrame(ExtendedAssembler *assembler, Register context)
{
    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Label processLazyDeopt;
    Label exit;
    Register frameTypeRegister = __ TempRegister1();

    __ Ldr(frameTypeRegister, MemoryOperand(context, AsmStackContext::GetIsFrameLazyDeoptOffset(false)));
    __ Cmp(frameTypeRegister, Immediate(0));
    __ B(Condition::NE, &processLazyDeopt);
    {
        __ Mov(frameTypeRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME)));
        __ B(&exit);
    }
    __ Bind(&processLazyDeopt);
    {
        __ Mov(frameTypeRegister, Immediate((static_cast<uint64_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME) |
            (1ULL << FrameIterator::LAZY_DEOPT_FLAG_BIT))));
    }
    __ Bind(&exit);
    // 2 : return addr & frame type
    __ Stp(frameTypeRegister, x30, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2 : prevSp & pc
    __ Stp(xzr, fp, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(24));  // 24: skip frame type, prevSp, pc

    if (!assembler->FromInterpreterHandler()) {
        __ CalleeSave();
    }
}

void OptimizedCall::DeoptHandlerAsm(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(DeoptHandlerAsm));
    __ PushFpAndLr();
    Register frameType = x3;
    Register glueReg = x0;

    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::ASM_BRIDGE_FRAME)));
    __ Stp(glueReg, frameType, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
    __ CalleeSave();

    Register deoptType = x1;
    Register maybeAcc = x2;
    Register argC = x3;
    Register runtimeId = x4;
    __ And(deoptType, deoptType, LogicalImmediate::Create(0x00000000FFFFFFFFULL, X_REG_SIZE));
    __ Orr(deoptType, deoptType, LogicalImmediate::Create(JSTaggedValue::TAG_INT, X_REG_SIZE));
    __ Stp(deoptType, maybeAcc, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(argC, Immediate(2)); // 2: argc
    __ Mov(runtimeId, Immediate(RTSTUB_ID(DeoptHandler)));
    __ Stp(runtimeId, argC, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
    __ Add(sp, sp, Immediate(2 * DOUBLE_SLOT_SIZE)); // 2: skip runtimeId, argc, depth, shiftLen

    __ CalleeRestore();
    Register context = x2;
    __ Mov(context, x0);
    __ Ldr(glueReg, MemoryOperand(sp, 0));

    Register ret = x0;
    Label stackOverflow;
    __ Cmp(ret, Immediate(JSTaggedValue::VALUE_EXCEPTION));
    __ B(Condition::EQ, &stackOverflow);

    Label target;
    Register temp = x1;
    __ Ldr(fp, MemoryOperand(context, AsmStackContext::GetCallerFpOffset(false)));
    __ Ldr(temp, MemoryOperand(context, AsmStackContext::GetCallFrameTopOffset(false)));
    __ Mov(sp, temp);
    __ Ldr(x30, MemoryOperand(context, AsmStackContext::GetReturnAddressOffset(false)));

    DeoptPushAsmInterpBridgeFrame(assembler, context);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    DeoptEnterAsmInterpOrBaseline(assembler);

    __ Bind(&stackOverflow);
    {
        __ Mov(runtimeId, Immediate(RTSTUB_ID(ThrowStackOverflowException)));
        // 2 : 2 means pair
        __ Stp(runtimeId, xzr, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
        __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
        __ Add(sp, sp, Immediate(2 * DOUBLE_SLOT_SIZE)); // 2: skip runtimeId&argc glue&type
        __ RestoreFpAndLr();
        __ Ret();
    }
}
#undef __
}  // panda::ecmascript::aarch64
