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

#include "ecmascript/compiler/trampoline/x64/common_call.h"

#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/message_string.h"

namespace panda::ecmascript::x64 {
#define __ assembler->

// * uint64_t JSFunctionEntry(uintptr_t glue, uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t prevFp)
// * Arguments:
//        %rdi - glue
//        %rsi - actualNumArgs
//        %rdx - argV
//        %rcx - prevFp
//        %r8 - needPushArgv
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
    Register glueReg = rdi;
    Register argv = rdx;
    Register prevFpReg = rcx;
    Register needPushArgv = r8;
    Label lJSCallWithArgVAndPushArgv;
    Label lPopFrame;
    PushJSFunctionEntryFrame(assembler, prevFpReg);
    __ Movq(argv, rbx);
    __ Movq(needPushArgv, r12);
    __ Movq(Operand(rbx, 0), rdx);
    __ Movq(Operand(rbx, FRAME_SLOT_SIZE), rcx);
    __ Movq(Operand(rbx, DOUBLE_SLOT_SIZE), r8);
    __ Addq(TRIPLE_SLOT_SIZE, rbx);
    __ Movq(rbx, r9);
    __ Cmp(1, r12);
    __ Je(&lJSCallWithArgVAndPushArgv);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgV), false);
    __ Jmp(&lPopFrame);

    __ Bind(&lJSCallWithArgVAndPushArgv);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgVAndPushArgv), false);

    __ Bind(&lPopFrame);
    __ Popq(prevFpReg);
    __ Addq(FRAME_SLOT_SIZE, rsp); // 8: frame type
    __ Popq(rbp);
    __ Popq(glueReg); // caller restore
    __ PopCppCalleeSaveRegisters(); // callee restore
    __ Movq(prevFpReg, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false)));
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
//               |--------------------------|
//               |       argc               |
//               |--------------------------| ---------------
//               |       returnAddr         |               ^
//      sp ----> |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|   OptimizedJSFunctionFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------

void OptimizedCall::OptimizedCallAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedCallAndPushArgv));
    Register jsFuncReg = rdi;
    Register method = r9;
    Register codeAddrReg = rsi;

    auto funcSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;  // 1: return addr
    __ Movq(Operand(rsp, funcSlotOffset * FRAME_SLOT_SIZE), jsFuncReg); // sp + 24 get jsFunc
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Mov(Operand(jsFuncReg, JSFunctionBase::CODE_ENTRY_OFFSET), codeAddrReg);

    Register methodCallField = rcx;
    __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField); // get call field
    __ Shr(MethodLiteral::NumArgsBits::START_BIT, methodCallField);
    __ Andl(((1LU <<  MethodLiteral::NumArgsBits::SIZE) - 1), methodCallField);
    __ Addl(NUM_MANDATORY_JSFUNC_ARGS, methodCallField); // add mandatory argumentr

    __ Movl(Operand(rsp, FRAME_SLOT_SIZE), rdx); // argc rdx
    __ Movq(rsp, r8);
    Register argvReg = r8;

    __ Addq(funcSlotOffset * FRAME_SLOT_SIZE, argvReg); // skip return addr, argc and agv

    Register expectedNumArgsReg = rcx;
    Register actualNumArgsReg = rdx;

    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
    // callee save
    __ Pushq(r14);
    __ Pushq(rbx);
    __ Pushq(rax);

    Label lCopyExtraAument1;
    Label lCopyLoop1;
    Label lCopyLoop2;
    Label lPopFrame1;
    Label pushUndefined;
    Label commonCall;

    __ Cmpq(expectedNumArgsReg, actualNumArgsReg);
    __ Jb(&pushUndefined);
    // 16 bytes align check
    __ Movl(actualNumArgsReg, r14);
    __ Testb(1, r14);
    __ Je(&lCopyLoop2);
    __ Pushq(0);

    __ Bind(&lCopyLoop2);
    __ Movq(Operand(argvReg, r14, Scale::Times8, -FRAME_SLOT_SIZE), rbx); // -8: stack index
    __ Pushq(rbx);
    __ Addq(-1, r14);
    __ Jne(&lCopyLoop2);
    __ Movl(actualNumArgsReg, r14);
    __ Jmp(&commonCall);

    __ Bind(&pushUndefined);
    // 16 bytes align check
    __ Movl(expectedNumArgsReg, r14);
    __ Testb(1, r14);
    __ Je(&lCopyExtraAument1);
    __ Pushq(0);

    __ Bind(&lCopyExtraAument1); // copy undefined value to stack
    __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
    __ Addq(-1, expectedNumArgsReg);
    __ Cmpq(actualNumArgsReg, expectedNumArgsReg);
    __ Ja(&lCopyExtraAument1);

    __ Bind(&lCopyLoop1);
    __ Movq(Operand(argvReg, expectedNumArgsReg, Scale::Times8, -FRAME_SLOT_SIZE), rbx); // -8: stack index
    __ Pushq(rbx);
    __ Addq(-1, expectedNumArgsReg);
    __ Jne(&lCopyLoop1);
    __ Jmp(&commonCall);

    __ Bind(&commonCall);
    __ Pushq(rsp); // actual argv
    __ Pushq(actualNumArgsReg); // actual argc

    __ Callq(codeAddrReg); // then call jsFunction
    __ Leaq(Operand(r14, Scale::Times8, 0), codeAddrReg);
    __ Addq(codeAddrReg, rsp);
    __ Addq(FRAME_SLOT_SIZE, rsp); // skip actualNumArgsReg
    __ Addq(FRAME_SLOT_SIZE, rsp); // skip argvReg
    __ Testb(1, r14); // stack 16bytes align check
    __ Je(&lPopFrame1);
    __ Addq(8, rsp); // 8: align byte

    __ Bind(&lPopFrame1);
    __ Addq(8, rsp); // 8: skip rax
    __ Popq(rbx);
    __ Popq(r14);
    __ Addq(FRAME_SLOT_SIZE, rsp); // skip frame type
    __ Pop(rbp);
    __ Ret();
}

void OptimizedCall::OptimizedCallAsmInterpreter(ExtendedAssembler *assembler)
{
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Callq(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_FROM_AOT, FrameTransitionType::OTHER_TO_OTHER);
}

// * uint64_t CallBuiltinTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...)
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
//          |       frameType          |                 |
//          |--------------------------|                 |
//          |       align byte         |                 v
//          +--------------------------+ -----------------

void OptimizedCall::RemoveArgv(ExtendedAssembler *assembler, Register temp)
{
    // remove argv
    __ Movq(Operand(rsp, FRAME_SLOT_SIZE), temp);
    __ Movq(temp, Operand(rsp, DOUBLE_SLOT_SIZE)); // argc -> argv
    __ Movq(Operand(rsp, 0), temp);
    __ Movq(temp, Operand(rsp, FRAME_SLOT_SIZE)); // returnAddr -> argc
    __ Addq(FRAME_SLOT_SIZE, rsp); // skip argv
}

void OptimizedCall::CallBuiltinTrampoline(ExtendedAssembler *assembler, Register temp)
{
    Register glueReg = rax;
    Register nativeCode = rsi;

    // remove argv
    RemoveArgv(assembler, temp);

    __ Movq(Operand(rsp, 0), rdx);
    __ Movq(glueReg, Operand(rsp, 0));
    __ Push(rdx);

    AsmInterpreterCall::PushBuiltinFrame(assembler, glueReg, FrameType::BUILTIN_CALL_LEAVE_FRAME);
    __ Leaq(Operand(rbp, DOUBLE_SLOT_SIZE), rdi); // 2: skip rbp & return Addr
    __ PushAlignBytes();
    AsmInterpreterCall::CallNativeInternal(assembler, nativeCode);
    __ Movq(Operand(rsp, DOUBLE_SLOT_SIZE), temp); // argc
    __ Movq(Immediate(0), Operand(rsp, DOUBLE_SLOT_SIZE)); // argv -> argc
    __ Movq(temp, Operand(rsp, FRAME_SLOT_SIZE)); // argc -> thread

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
    // remove argv
    RemoveArgv(assembler, temp);

    __ Movq(Operand(rsp, 0), temp);
    __ Movq(glue, Operand(rsp, 0));
    __ Push(temp);

    // push rbp & frameType
    AsmInterpreterCall::PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_CALL_LEAVE_FRAME);
    __ Push(argv);
    __ Callq(builtinStub);
    // resume rsp
    __ Movq(rbp, rsp);
    __ Pop(rbp);
    __ Movq(Operand(rsp, DOUBLE_SLOT_SIZE), temp); // argc
    __ Movq(Immediate(0), Operand(rsp, DOUBLE_SLOT_SIZE)); // 0 -> argc
    __ Movq(temp, Operand(rsp, FRAME_SLOT_SIZE)); // argc -> thread
    __ Ret();
}

// * uint64_t JSProxyCallInternalWithArgV(uintptr_t glue, JSTaggedType calltarget)
// * c++ calling convention call js function
// * Arguments:
//        %rdi - glue
//        %rsi - calltarget

void OptimizedCall::JSProxyCallInternalWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSProxyCallInternalWithArgV));
    Register ccGlueReg = rdi;
    Register jsccGlueReg = rax;
    Register callTarget = rsi;
    __ Movq(ccGlueReg, jsccGlueReg);  // c++ calling convention as webkit_jscc calling convention
    auto funcSlotOffSet = kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;
    __ Movq(callTarget, Operand(rsp, funcSlotOffSet * FRAME_SLOT_SIZE));  // update callTarget slot
    GenJSCall(assembler, false);
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
//               |--------------------------| ---------------
//               |       returnAddr         |               ^
//      sp ----> |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|   OptimizedJSFunctionFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------
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

void OptimizedCall::GenJSCall(ExtendedAssembler *assembler, bool isNew)
{
    Label jsCall;
    Label lJSCallStart;
    Label lNotJSFunction;
    Label lNonCallable;
    Label lJSFunctionCall;
    Label lJSBoundFunction;
    Label lJSProxy;
    Label lCallNativeMethod;
    Label lCallNativeCpp;
    Label lCallNativeBuiltinStub;
    Register glueReg = rax;
    __ Bind(&jsCall);
    {
        __ Movq(glueReg, rdi);
        glueReg = rdi;
        auto funcSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;
        __ Movq(Operand(rsp, funcSlotOffset * FRAME_SLOT_SIZE), rax); // sp + 24 get jsFunc
    }
    __ Bind(&lJSCallStart);
    Register jsFuncReg = rax;
    {
        JSCallCheck(assembler, jsFuncReg, &lNonCallable, &lNotJSFunction, &lJSFunctionCall);
    }

    __ Bind(&lNotJSFunction);
    {
        __ Cmpb(static_cast<uint8_t>(JSType::JS_BOUND_FUNCTION), rax); // IsBoundFunction
        __ Je(&lJSBoundFunction);
        __ Cmpb(static_cast<uint8_t>(JSType::JS_PROXY), rax); // IsJsProxy
        __ Je(&lJSProxy);
    }

    __ Bind(&lNonCallable);
    {
        ThrowNonCallableInternal(assembler, glueReg);
    }

    __ Bind(&lJSFunctionCall);
    jsFuncReg = rsi;
    Register argc = r8;
    Register methodCallField = rcx;
    Register method = rdx;
    Register argV = r9;
    {
        Label lCallConstructor;
        Label lNotClass;
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
        __ Movl(Operand(rsp, FRAME_SLOT_SIZE), argc); // skip return addr
        __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField); // get call field
        __ Btq(MethodLiteral::IsNativeBit::START_BIT, methodCallField); // is native
        __ Jb(&lCallNativeMethod);
        if (!isNew) {
            __ Btq(JSHClass::IsClassConstructorOrPrototypeBit::START_BIT, rax); // is CallConstructor
            __ Jnb(&lNotClass);
            __ Btq(JSHClass::ConstructorBit::START_BIT, rax); // is CallConstructor
            __ Jb(&lCallConstructor);
        }
        __ Bind(&lNotClass);
        __ Movq(rsp, argV);
        auto argvSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;  // 1: return addr
        __ Addq(argvSlotOffset * FRAME_SLOT_SIZE, argV); // skip return addr, argc and argv
        __ Subq(Immediate(kungfu::ArgumentAccessor::GetFixArgsNum()), argc);
        // argv + 24 get asm interpreter argv
        __ Addq(kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE, argV);
        OptimizedCallAsmInterpreter(assembler);
        __ Bind(&lCallConstructor);
        {
            __ Pushq(rbp);
            __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)); // set frame type
            __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
            __ Pushq(0); // PushAlign
            __ Pushq(0); // argc
            __ Pushq(RTSTUB_ID(ThrowCallConstructorException)); // runtime id
            __ Movq(glueReg, rax); // glue
            __ Movq(kungfu::RuntimeStubCSigns::ID_CallRuntime, r10);
            __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
            __ Callq(r10); // call CallRuntime
            __ Addq(4 * FRAME_SLOT_SIZE, rsp); // 4: sp + 32 argv
            __ Pop(rbp);
            __ Ret();
        }
    }

    __ Bind(&lCallNativeMethod);
    {
        Register nativePointer = rsi;
        method = rax;
        __ Movq(jsFuncReg, rdx);
        __ Mov(Operand(rdx, JSFunctionBase::METHOD_OFFSET), method);  // get method
        __ Mov(Operand(rdx, JSFunctionBase::CODE_ENTRY_OFFSET), nativePointer);  // native pointer
        __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField);  // get call field
        __ Btq(MethodLiteral::IsFastBuiltinBit::START_BIT, methodCallField);  // is builtin stub

        if (!isNew) {
            __ Jnb(&lCallNativeCpp);
            __ Cmpl(NUM_MANDATORY_JSFUNC_ARGS + 3, argc);  // 3:call0, call1, call2, call3
            __ Jbe(&lCallNativeBuiltinStub);
        } else {
            __ Jb(&lCallNativeBuiltinStub);
        }
    }

    __ Bind(&lCallNativeCpp);
    {
        __ Movq(glueReg, rax);
        CallBuiltinTrampoline(assembler, r11);
    }

    __ Bind(&lCallNativeBuiltinStub);
    {
        Register methodExtraLiteralInfo = rax;
        __ Mov(Operand(method, Method::EXTRA_LITERAL_INFO_OFFSET), methodExtraLiteralInfo);  // get extra literal
        __ Shr(MethodLiteral::BuiltinIdBits::START_BIT, methodExtraLiteralInfo);
        __ Andl(((1LU <<  MethodLiteral::BuiltinIdBits::SIZE) - 1), methodExtraLiteralInfo);  // get builtin stub id
        if (!isNew) {
            __ Cmpl(BUILTINS_STUB_ID(BUILTINS_CONSTRUCTOR_STUB_FIRST), methodExtraLiteralInfo);
            __ Jnb(&lCallNativeCpp);
        }

        __ Movq(glueReg, rdi);
        __ Movq(methodExtraLiteralInfo, r10);
        __ Movq(Operand(glueReg, r10, Times8, JSThread::GlueData::GetBuiltinsStubEntriesOffset(false)), r10);

        __ Movq(argc, r9);
        __ Movq(Operand(rsp, QUADRUPLE_SLOT_SIZE), rcx);              // newTarget
        __ Movq(Operand(rsp, QUINTUPLE_SLOT_SIZE), r8);               // this
        __ Subq(NUM_MANDATORY_JSFUNC_ARGS, r9);                       // argc

        Label lCall0;
        Label lCall1;
        Label lCall2;
        Label lCall3;
        argV = rax;

        __ Movq(rsp, argV);
        auto argvSlotOffset = kungfu::ArgumentAccessor::GetFixArgsNum() +
            kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;  // 1: return addr
        __ Addq(argvSlotOffset *FRAME_SLOT_SIZE, argV);
        if (!isNew) {
            PushAsmBridgeFrame(assembler);

            __ Cmpl(0, r9);  // 0: callarg0
            __ Je(&lCall0);
            __ Cmpl(1, r9);  // 1: callarg1
            __ Je(&lCall1);
            __ Cmpl(2, r9);  // 2: callarg2
            __ Je(&lCall2);
            __ Cmpl(3, r9);  // 3: callarg3
            __ Je(&lCall3);

            __ Bind(&lCall0);
            {
                __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
                __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
                __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
                __ Callq(r10);
                __ Addq(QUADRUPLE_SLOT_SIZE, rsp);
                __ Pop(rbp);
                __ Ret();
            }

            __ Bind(&lCall1);
            {
                __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
                __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
                __ Movq(Operand(argV, 0), r11);                     // arg0
                __ Pushq(r11);
                __ Callq(r10);
                __ Addq(QUADRUPLE_SLOT_SIZE, rsp);
                __ Pop(rbp);
                __ Ret();
            }

            __ Bind(&lCall2);
            {
                __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
                __ Movq(Operand(argV, FRAME_SLOT_SIZE), r11);        // arg1
                __ Pushq(r11);
                __ Movq(Operand(argV, 0), r11);                      // arg0
                __ Pushq(r11);
                __ Callq(r10);
                __ Addq(QUADRUPLE_SLOT_SIZE, rsp);
                __ Pop(rbp);
                __ Ret();
            }

            __ Bind(&lCall3);
            {
                __ Movq(Operand(argV, DOUBLE_SLOT_SIZE), r11);     // arg2
                __ Pushq(r11);
                __ Movq(Operand(argV, FRAME_SLOT_SIZE), r11);      // arg1
                __ Pushq(r11);
                __ Movq(Operand(argV, 0), r11);                    // arg0
                __ Pushq(r11);
                __ Callq(r10);
                __ Addq(QUADRUPLE_SLOT_SIZE, rsp);
                __ Pop(rbp);
                __ Ret();
            }
        } else {
            CallBuiltinConstructorStub(assembler, r10, argV, glueReg, r11);
            __ Int3();
        }
    }

    __ Bind(&lJSBoundFunction);
    {
        JSBoundFunctionCallInternal(assembler, jsFuncReg, &jsCall);
    }
    __ Bind(&lJSProxy);
    {
        Register nativePointer = rsi;
        __ Mov(Operand(jsFuncReg, JSProxy::METHOD_OFFSET), method);
        __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField);
        __ Mov(Operand(rsp, FRAME_SLOT_SIZE), argc);
        __ Movq(Operand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET), nativePointer);
        __ Jmp(&lCallNativeCpp);
    }
}

// After the callee function of common aot call deopt, use this bridge to deal with this aot call.
// calling convention: webkit_jsc
// Input structure:
// %rax - glue
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
// |--------------------------|
// |       returnAddr         |
// +--------------------------+ <---- sp
void OptimizedCall::AOTCallToAsmInterBridge(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(AOTCallToAsmInterBridge));
    // params of c++ calling convention
    Register glueReg = rdi;
    Register jsFuncReg = rsi;
    Register method = rdx;
    Register methodCallField = rcx;
    Register argc = r8;
    Register argV = r9;

    __ Movq(rax, glueReg);
    __ Movq(Operand(rsp, TRIPLE_SLOT_SIZE), jsFuncReg);
    __ Movq(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Movq(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField); // get call field
    __ Movl(Operand(rsp, FRAME_SLOT_SIZE), argc); // skip return addr
    __ Subq(Immediate(kungfu::ArgumentAccessor::GetFixArgsNum()), argc);
    __ Movq(rsp, argV);
    auto argvSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;  // 1: return addr
    __ Addq(argvSlotOffset * FRAME_SLOT_SIZE, argV); // skip return addr and argc
    __ Addq(kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE, argV);
    OptimizedCallAsmInterpreter(assembler);
}

// After the callee function of fast aot call deopt, use this bridge to deal with this fast aot call.
// Notice: no argc and new-target params compared with not-fast aot call because these params are not needed
// according to bytecode-analysis.
// Intruduction: use expected argc as actual argc below for these reasons:
// 1) when expected argc == actual argc, pass.
// 2) when expected argc > actual argc, undefineds have been pushed in OptimizedFastCallAndPushArgv.
// 3) when expected argc < actual argc, redundant params are useless according to bytecode-analysis, just abandon them.
// calling convention: c++ calling convention
// Input structure:
// %rdi - glue
// %rsi - call-target
// %rdx - this
// %rcx - arg0
// %r8  - arg1
// %r9  - arg2
// stack:
// +--------------------------+
// |        arg[N-1]          |
// +--------------------------+
// |       ...                |
// +--------------------------+
// |       arg[3]             |
// +--------------------------+
// |       returnAddr         |
// |--------------------------| <---- sp
void OptimizedCall::FastCallToAsmInterBridge(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(FastCallToAsmInterBridge));
    // Input
    Register glueReg = rdi;
    Register jsFuncReg = rsi;
    Register thisReg = rdx;
    Register maybeArg0 = rcx;
    Register maybeArg1 = r8;
    Register maybeArg2 = r9;

    // Add a bridge frame to protect the stack map
    PushAsmBridgeFrame(assembler);

    Register tempMethod = __ AvailableRegister1();
    Register tempCallField = __ AvailableRegister2();

    __ Movq(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), tempMethod);
    __ Movq(Operand(tempMethod, Method::CALL_FIELD_OFFSET), tempCallField);
    // get expected num Args
    Register tempArgc = tempCallField;
    __ Shr(MethodLiteral::NumArgsBits::START_BIT, tempArgc);
    __ Andl(((1LU <<  MethodLiteral::NumArgsBits::SIZE) - 1), tempArgc);

    {
        [[maybe_unused]] TempRegisterScope scope(assembler);
        Register startSp = __ TempRegister();
        __ Movq(rsp, startSp);

        Label lCall0;
        Label lCall1;
        Label lCall2;
        Label lCall3;
        Label lPushCommonRegs;

        __ Cmpl(0, tempArgc);  // 0: callarg0
        __ Je(&lCall0);
        __ Cmpl(1, tempArgc);  // 1: callarg1
        __ Je(&lCall1);
        __ Cmpl(2, tempArgc);  // 2: callarg2
        __ Je(&lCall2);
        __ Cmpl(3, tempArgc);  // 3: callarg3
        __ Je(&lCall3);
        // default: more than 3 args
        {
            __ Subq(Immediate(3), tempArgc);  // 3: the first 3 args are not on stack
            __ Addq(Immediate(TRIPLE_SLOT_SIZE), startSp);  // skip bridge frame and return addr
            CopyArgumentWithArgV(assembler, tempArgc, startSp);
            __ Subq(Immediate(TRIPLE_SLOT_SIZE), startSp);
            __ Jmp(&lCall3);
        }

        __ Bind(&lCall0);
        {
            __ Jmp(&lPushCommonRegs);
        }

        __ Bind(&lCall1);
        {
            __ Pushq(maybeArg0);
            __ Jmp(&lPushCommonRegs);
        }

        __ Bind(&lCall2);
        {
            __ Pushq(maybeArg1);
            __ Pushq(maybeArg0);
            __ Jmp(&lPushCommonRegs);
        }

        __ Bind(&lCall3);
        {
            __ Pushq(maybeArg2);
            __ Pushq(maybeArg1);
            __ Pushq(maybeArg0);
            __ Jmp(&lPushCommonRegs);
        }

        __ Bind(&lPushCommonRegs);
        {
            __ Pushq(thisReg);
            __ Pushq(JSTaggedValue::VALUE_UNDEFINED); // newTarget
            __ Pushq(jsFuncReg);
            // fall through
        }

        // params of c++ calling convention
        glueReg = rdi;
        jsFuncReg = rsi;
        Register method = rdx;
        Register methodCallField = rcx;
        Register argc = r8;
        Register argV = r9;

        // reload and prepare args for JSCallCommonEntry
        __ Movq(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method);
        __ Movq(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField);
        __ Movq(methodCallField, argc);
        __ Shr(MethodLiteral::NumArgsBits::START_BIT, argc);
        __ Andl(((1LU <<  MethodLiteral::NumArgsBits::SIZE) - 1), argc);
        __ Movq(rsp, argV);
        __ Addq(Immediate(TRIPLE_SLOT_SIZE), argV);  // skip func, newtarget and this

        __ Pushq(startSp);  // used for resume rsp
    }

    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Callq(&target);
    {
        PopAsmInterpBridgeFrame(assembler);
        Register startSp = __ AvailableRegister1();
        __ Popq(startSp);
        __ Movq(startSp, rsp);
        PopAsmBridgeFrame(assembler);
        __ Ret();
    }
    __ Bind(&target);
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_FROM_AOT, FrameTransitionType::OTHER_TO_OTHER);
}

void OptimizedCall::PushAsmBridgeFrame(ExtendedAssembler *assembler)
{
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::ASM_BRIDGE_FRAME));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
}

void OptimizedCall::PopAsmBridgeFrame(ExtendedAssembler *assembler)
{
    __ Addq(FRAME_SLOT_SIZE, rsp);  // skip type
    __ Popq(rbp);
}

void OptimizedCall::JSCallCheck(ExtendedAssembler *assembler, Register jsFuncReg,
                                Label *lNonCallable, Label *lNotJSFunction, Label *lJSFunctionCall)
{
    __ Movabs(JSTaggedValue::TAG_INT, rdx); // IsTaggedInt
    __ And(jsFuncReg, rdx);
    __ Cmp(0x0, rdx);
    __ Jne(lNonCallable);
    __ Cmp(0x0, jsFuncReg); // IsHole
    __ Je(lNonCallable);
    __ Movabs(JSTaggedValue::TAG_SPECIAL, rdx);
    __ And(jsFuncReg, rdx);  // IsSpecial
    __ Cmp(0x0, rdx);
    __ Jne(lNonCallable);

    __ Movq(jsFuncReg, rsi); // save jsFunc
    __ Movq(Operand(jsFuncReg, JSFunction::HCLASS_OFFSET), rax); // get jsHclass
    Register maskRegister = rdx;
    __ Movabs(TaggedObject::GC_STATE_MASK, maskRegister);
    __ And(maskRegister, rax);
    Register jsHclassReg = rax;
    __ Movl(Operand(jsHclassReg, JSHClass::BIT_FIELD_OFFSET), rax);
    __ Btl(JSHClass::CallableBit::START_BIT, rax); // IsCallable
    __ Jnb(lNonCallable);

    __ Cmpb(static_cast<int32_t>(JSType::JS_FUNCTION_FIRST), rax);
    __ Jb(lNotJSFunction);
    __ Cmpb(static_cast<int32_t>(JSType::JS_FUNCTION_LAST), rax);
    __ Jbe(lJSFunctionCall); // objecttype in (0x04 ~ 0x0c)
}

void OptimizedCall::ThrowNonCallableInternal(ExtendedAssembler *assembler, Register glueReg)
{
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)); // set frame type
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
    __ Movq(MessageString::Message_NonCallable, rax);
    __ Movabs(JSTaggedValue::TAG_INT, r10);
    __ Orq(r10, rax);
    __ Pushq(rax); // message id
    __ Pushq(1); // argc
    __ Pushq(RTSTUB_ID(ThrowTypeError)); // runtime id
    __ Movq(glueReg, rax); // glue
    __ Movq(kungfu::RuntimeStubCSigns::ID_CallRuntime, r10);
    __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
    __ Callq(r10); // call CallRuntime
    __ Movabs(JSTaggedValue::VALUE_EXCEPTION, rax); // return exception
    __ Addq(4 * FRAME_SLOT_SIZE, rsp); // 4: sp + 32 argv
    __ Pop(rbp);
    __ Ret();
}

void OptimizedCall::JSBoundFunctionCallInternal(ExtendedAssembler *assembler, Register jsFuncReg, Label *jsCall)
{
    Label lAlign16Bytes2;
    Label lCopyBoundArgument;
    Label lCopyArgument2;
    Label lPushCallTarget;
    Label lCopyBoundArgumentLoop;
    Label lPopFrame2;
    Label slowCall;
    Label aotCall;
    Label popArgs;
    Label isJsFunc;
    Label isNotClass;
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
    __ Pushq(r10); // callee save
    __ Movq(rsp, rdx);
    __ Addq(QUADRUPLE_SLOT_SIZE, rdx); // skip return addr, prevFp, frame type and callee save
    __ Mov(Operand(rdx, 0), rax); // get origin argc
    __ Mov(Operand(rdx, FRAME_SLOT_SIZE), r9); // get origin argv
    __ Movq(rax, r10);
    // get bound target
    __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_ARGUMENTS_OFFSET), rcx);
    // get bound length
    __ Mov(Operand(rcx, TaggedArray::LENGTH_OFFSET), rcx);
    __ Addq(rcx, r10);

    // 16 bytes align check
    __ Testb(1, r10);
    __ Je(&lAlign16Bytes2);
    __ PushAlignBytes(); // push zero to align 16 bytes stack

    __ Bind(&lAlign16Bytes2);
    {
        __ Subq(NUM_MANDATORY_JSFUNC_ARGS, rax);
        __ Cmp(0, rax);
        __ Je(&lCopyBoundArgument);
    }

    __ Bind(&lCopyArgument2);
    {
        __ Movq(Operand(rdx, rax, Scale::Times8,
            (kungfu::ArgumentAccessor::GetFixArgsNum() + 1) * FRAME_SLOT_SIZE), rcx); // argv
        __ Pushq(rcx);
        __ Addq(-1, rax);
        __ Jne(&lCopyArgument2);
    }

    __ Bind(&lCopyBoundArgument);
    {
        // get bound target
        __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_ARGUMENTS_OFFSET), rdx);
        // get bound length
        __ Mov(Operand(rdx, TaggedArray::LENGTH_OFFSET), rax);
        __ Addq(TaggedArray::DATA_OFFSET, rdx);
        __ Cmp(0, rax);
        __ Je(&lPushCallTarget);
    }
    __ Bind(&lCopyBoundArgumentLoop);
    {
        __ Addq(-1, rax);
        __ Movq(Operand(rdx, rax, Scale::Times8, 0), rcx);
        __ Pushq(rcx);
        __ Jne(&lCopyBoundArgumentLoop);
    }
    __ Bind(&lPushCallTarget);
    {
        __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_THIS_OFFSET), r8); // thisObj
        __ Pushq(r8);
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED); // newTarget
        __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_TARGET_OFFSET), rax); // callTarget
        __ Pushq(rax);
        __ Pushq(r9);
        __ Pushq(r10); // push actual arguments
    }
    JSCallCheck(assembler, rax, &slowCall, &slowCall, &isJsFunc); // jsfunc -> rsi hclassfiled -> rax
    __ Jmp(&slowCall);
    Register jsfunc = rsi;
    Register compiledCodeFlag = rcx;
    __ Bind(&isJsFunc);
    {
        __ Btq(JSHClass::IsClassConstructorOrPrototypeBit::START_BIT, rax); // is CallConstructor
        __ Jnb(&isNotClass);
        __ Btq(JSHClass::ConstructorBit::START_BIT, rax);
        __ Jb(&slowCall);
        __ Bind(&isNotClass);
        __ Movzwq(Operand(rsi, JSFunctionBase::BIT_FIELD_OFFSET), compiledCodeFlag); // compiled code flag
        __ Btq(JSFunctionBase::IsCompiledCodeBit::START_BIT, compiledCodeFlag); // is compiled code
        __ Jnb(&slowCall);
        __ Bind(&aotCall);
        {
            // output: glue:rdi argc:rsi calltarget:rdx argv:rcx this:r8 newtarget:r9
            __ Movq(jsfunc, rdx);
            __ Movq(r10, rsi);
            auto funcSlotOffSet = kungfu::ArgumentAccessor::GetFixArgsNum() +
                                  kungfu::ArgumentAccessor::GetExtraArgsNum();
            __ Leaq(Operand(rsp, funcSlotOffSet * FRAME_SLOT_SIZE), rcx); // 5: skip argc and argv func new this
            __ Movq(JSTaggedValue::VALUE_UNDEFINED, r9);
            __ Movq(kungfu::CommonStubCSigns::JsBoundCallInternal, r10);
            __ Movq(Operand(rdi, r10, Scale::Times8, JSThread::GlueData::GetCOStubEntriesOffset(false)), rax);
            __ Callq(rax); // call JSCall
            __ Jmp(&popArgs);
        }
    }
    __ Bind(&slowCall);
    {
        __ Movq(rdi, rax);
        __ Callq(jsCall); // call JSCall
        __ Jmp(&popArgs);
    }
    __ Bind(&popArgs);
    {
        __ Pop(r10);
        __ Pop(r9);
        __ Leaq(Operand(r10, Scale::Times8, 0), rcx); // 8: disp
        __ Addq(rcx, rsp);
        __ Testb(1, r10);  // stack 16bytes align check
        __ Je(&lPopFrame2);
        __ Addq(8, rsp); // 8: align byte
    }
    __ Bind(&lPopFrame2);
    {
        __ Pop(r10);
        __ Addq(8, rsp); // 8: sp + 8
        __ Pop(rbp);
        __ Ret();
    }
}

// * uint64_t CallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, uintptr_t arg0, ...)
// * webkit_jscc calling convention call runtime_id's runtime function(c-abi)
// * Arguments:
//         %rax - glue
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
    __ BindAssemblerStub(RTSTUB_ID(CallRuntime));
    __ Pushq(rbp);
    __ Movq(rsp, Operand(rax, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Pushq(static_cast<int32_t>(FrameType::LEAVE_FRAME));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);  // 8: skip frame type

    __ Pushq(r10);
    __ Pushq(rdx);
    __ Pushq(rax);

    __ Movq(rbp, rdx);
    // 2: rbp & return address
    __ Addq(2 * FRAME_SLOT_SIZE, rdx);

    __ Movq(Operand(rdx, 0), r10);
    __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
    __ Movq(rax, rdi);
    // 8: argc
    __ Movq(Operand(rdx, FRAME_SLOT_SIZE), rsi);
    // 2: argv
    __ Addq(2 * FRAME_SLOT_SIZE, rdx);
    __ Callq(r10);

    // 8: skip rax
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Popq(rdx);
    __ Popq(r10);

    // 8: skip frame type
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Popq(rbp);
    __ Ret();
}

// * uint64_t CallRuntimeWithArgv(uintptr_t glue, uint64_t runtime_id, uint64_t argc, uintptr_t argv)
// * cc calling convention call runtime_id's runtion function(c-abi)
// * Arguments:
//         %rdi - glue
//         %rsi - runtime_id
//         %edx - argc
//         %rcx - argv
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
    Register glueReg = rdi;
    Register runtimeIdReg = rsi;
    Register argcReg = rdx;
    Register argvReg = rcx;

    __ Movq(rsp, r8);
    Register returnAddrReg = r9;
    __ Movq(Operand(rsp, 0), returnAddrReg);
    __ Pushq(argvReg); // argv[]
    __ Pushq(argcReg); // argc
    __ Pushq(runtimeIdReg); // runtime_id
    __ Pushq(returnAddrReg); // returnAddr

    // construct leave frame
    __ Pushq(rbp);
    __ Movq(rsp, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false))); // save to thread->leaveFrame_
    __ Pushq(static_cast<int32_t>(FrameType::LEAVE_FRAME_WITH_ARGV));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);

    __ Movq(Operand(glueReg, runtimeIdReg, Scale::Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r9);
    __ Movq(argcReg, rsi); // argc
    __ Movq(argvReg, rdx); // argv
    __ Pushq(r8);
    __ Callq(r9);
    __ Popq(r8);
    __ Addq(FRAME_SLOT_SIZE, rsp); // 8: skip type
    __ Popq(rbp);
    __ Movq(r8, rsp);
    __ Ret();
}

void OptimizedCall::PushMandatoryJSArgs(ExtendedAssembler *assembler, Register jsfunc,
                                        Register thisObj, Register newTarget)
{
    __ Pushq(thisObj);
    __ Pushq(newTarget);
    __ Pushq(jsfunc);
}

// output expectedNumArgs (r14)
void OptimizedCall::PushArgsWithArgV(ExtendedAssembler *assembler, Register jsfunc,
                                     Register actualNumArgs, Register argV, Label *pushCallThis)
{
    Register expectedNumArgs(r14); // output
    Register tmp(rax);
    Label align16Bytes;
    Label copyArguments;
    // get expected num Args
    __ Movq(Operand(jsfunc, JSFunctionBase::METHOD_OFFSET), tmp);
    __ Movq(Operand(tmp, Method::CALL_FIELD_OFFSET), tmp);
    __ Shr(MethodLiteral::NumArgsBits::START_BIT, tmp);
    __ Andl(((1LU <<  MethodLiteral::NumArgsBits::SIZE) - 1), tmp);

    __ Mov(tmp, expectedNumArgs);
    __ Testb(1, expectedNumArgs);
    __ Jne(&align16Bytes);
    __ PushAlignBytes();

    __ Bind(&align16Bytes);
    {
        __ Cmpq(actualNumArgs, expectedNumArgs);
        __ Jbe(&copyArguments);
        __ Subq(actualNumArgs, tmp);
        PushUndefinedWithArgc(assembler, tmp);
    }
    __ Bind(&copyArguments);
    {
        __ Cmpq(actualNumArgs, expectedNumArgs);
        __ Movq(actualNumArgs, tmp);     // rax -> actualNumArgsReg
        __ CMovbe(expectedNumArgs, tmp);
        __ Cmpq(0, tmp);
        __ Je(pushCallThis);
        CopyArgumentWithArgV(assembler, tmp, argV);
    }
}

void OptimizedCall::PopJSFunctionArgs(ExtendedAssembler *assembler, Register expectedNumArgs)
{
    Label align16Bytes;
    __ Testb(1, expectedNumArgs);
    __ Je(&align16Bytes);
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Bind(&align16Bytes);
    __ Leaq(Operand(expectedNumArgs, Scale::Times8, 0), expectedNumArgs);
    __ Addq(DOUBLE_SLOT_SIZE, rsp); // 16: skip actual argc and actual argv
    __ Addq(expectedNumArgs, rsp);
}

void OptimizedCall::PushJSFunctionEntryFrame(ExtendedAssembler *assembler, Register prevFp)
{
    __ PushCppCalleeSaveRegisters();
    __ Pushq(rdi);

    // construct optimized entry frame
    __ Pushq(rbp);
    __ Pushq(static_cast<int64_t>(FrameType::OPTIMIZED_ENTRY_FRAME));
    __ Pushq(prevFp);
    // 2: skip prevFp and frameType
    __ Leaq(Operand(rsp, 2 * FRAME_SLOT_SIZE), rbp);
}

void OptimizedCall::PopJSFunctionEntryFrame(ExtendedAssembler *assembler, Register glue)
{
    Register prevFp(rsi);
    __ Popq(prevFp);
    __ Addq(FRAME_SLOT_SIZE, rsp); // 8: frame type
    __ Popq(rbp);
    __ Popq(glue); // caller restore
    __ PopCppCalleeSaveRegisters(); // callee restore
    __ Movq(prevFp, Operand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
}

// * uint64_t PushOptimizedUnfoldArgVFrame(uintptr_t glue, uint32_t argc, JSTaggedType calltarget,
//                                         JSTaggedType new, JSTaggedType this, JSTaggedType, argV[])
// * cc calling convention call js function()
// * arguments:
//              %rdi - glue
//              %rsi - argc
//              %rdx - call-target
//              %rcx - new-target
//              %r8  - this
//              %r9  - argv
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
    __ Pushq(rbp);
    // construct frame
    __ Pushq(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME));
    __ Pushq(callSiteSp);
    // 2: skip callSiteSp and frameType
    __ Leaq(Operand(rsp, 2 * FRAME_SLOT_SIZE), rbp);
}

void OptimizedCall::PopOptimizedUnfoldArgVFrame(ExtendedAssembler *assembler)
{
    Register sp(rsp);
    // 2 : 2 means pop call site sp and type
    __ Addq(Immediate(2 * FRAME_SLOT_SIZE), sp);
    __ Popq(rbp);
}

// * uint64_t JSCallWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType calltarget,
//                          JSTaggedType new, JSTaggedType this, argV)
// * cc calling convention call js function()
// * arguments:
//              %rdi - glue
//              %rsi - argc
//              %rdx - call-target
//              %rcx - new-target
//              %r8  - this
//              %r9  - argv
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
//               |--------------------------|   OptimizedJSFunctionFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------

void OptimizedCall::GenJSCallWithArgV(ExtendedAssembler *assembler, int id)
{
    Register sp(rsp);
    Register glue(rdi);
    Register actualNumArgs(rsi);
    Register jsfunc(rdx);
    Register newTarget(rcx);
    Register thisObj(r8);
    Register argV(r9);
    Register callsiteSp = __ AvailableRegister2();
    Label align16Bytes;
    Label pushCallThis;

    __ Movq(sp, callsiteSp);
    __ Addq(Immediate(FRAME_SLOT_SIZE), callsiteSp);   // 8 : 8 means skip pc to get last callsitesp
    PushOptimizedUnfoldArgVFrame(assembler, callsiteSp);
    __ Testb(1, actualNumArgs);
    __ Jne(&align16Bytes);
    __ PushAlignBytes();
    __ Bind(&align16Bytes);
    __ Cmp(Immediate(0), actualNumArgs);
    __ Jz(&pushCallThis);
    __ Mov(actualNumArgs, rax);
    CopyArgumentWithArgV(assembler, rax, argV);
    __ Bind(&pushCallThis);
    PushMandatoryJSArgs(assembler, jsfunc, thisObj, newTarget);
    __ Addq(Immediate(NUM_MANDATORY_JSFUNC_ARGS), actualNumArgs);
    __ Pushq(sp);
    __ Pushq(actualNumArgs);
    __ Movq(glue, rax);
    __ CallAssemblerStub(id, false);
    __ Mov(Operand(sp, 0), actualNumArgs);
    PopJSFunctionArgs(assembler, actualNumArgs);
    PopOptimizedUnfoldArgVFrame(assembler);
    __ Ret();
}

// * uint64_t JSCallWithArgVAndPushArgv(uintptr_t glue, uint32_t argc, JSTaggedType calltarget,
//                          JSTaggedType new, JSTaggedType this, argV)
// * cc calling convention call js function()
// * arguments:
//              %rdi - glue
//              %rsi - argc
//              %rdx - call-target
//              %rcx - new-target
//              %r8  - this
//              %r9  - argv
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
    Register jsFuncReg = rdi;
    Register method = r9;
    Register codeAddrReg = rsi;
    auto funcSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;  // 1: return addr
    __ Movq(Operand(rsp, funcSlotOffset * FRAME_SLOT_SIZE), jsFuncReg); // sp + 24 get jsFunc
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Mov(Operand(jsFuncReg, JSFunctionBase::CODE_ENTRY_OFFSET), codeAddrReg);
    __ Jmp(codeAddrReg);
}

// Input: %rdi - glue
//        %rsi - context
void OptimizedCall::DeoptEnterAsmInterpOrBaseline(ExtendedAssembler *assembler)
{
    // rdi
    Register glueRegister = __ GlueRegister();
    Register context = rsi;
    // rax, rdx, rcx, r8, r9, r10, r11 is free
    Register tempRegister = rax;
    Register opRegister = r10;
    Register outputCount = rdx;
    Register frameStateBase = rcx;
    Register depth = r11;
    Register hasExceptionRegister = r15;
    Label loopBegin;
    Label stackOverflow;
    Label gotoExceptionHandler;
    Label pushArgv;
    __ Movq(Operand(context, AsmStackContext::GetHasExceptionOffset(false)), hasExceptionRegister);
    __ Movq(Operand(context, AsmStackContext::GetInlineDepthOffset(false)), depth);
    __ Leaq(Operand(context, AsmStackContext::GetSize(false)), context);
    __ Movq(Immediate(0), r12);
    __ Bind(&loopBegin);
    __ Movq(Operand(context, 0), outputCount);
    __ Leaq(Operand(context, FRAME_SLOT_SIZE), frameStateBase);
    __ Cmpq(0, r12);
    __ Je(&pushArgv);
    __ Movq(rsp, r8);
    __ Addq(AsmInterpretedFrame::GetSize(false), r8);
    __ Leaq(Operand(frameStateBase, AsmInterpretedFrame::GetBaseOffset(false)), r10);
    __ Movq(r8, Operand(r10, InterpretedFrameBase::GetPrevOffset(false)));
    __ Testq(15, rsp);  // 15: low 4 bits must be 0b0000
    __ Jnz(&pushArgv);
    __ PushAlignBytes();
    __ Bind(&pushArgv);
    // update fp
    __ Movq(rsp, Operand(frameStateBase, AsmInterpretedFrame::GetFpOffset(false)));
    PushArgsWithArgvAndCheckStack(assembler, glueRegister, outputCount,
        frameStateBase, tempRegister, opRegister, &stackOverflow);
    __ Leaq(Operand(context, outputCount, Scale::Times8, FRAME_SLOT_SIZE), context);
    __ Addq(1, r12);
    __ Cmpq(r12, depth);
    __ Jae(&loopBegin);

    Register callTargetRegister = r8;
    Register methodRegister = r9;
    __ Movq(Operand(frameStateBase, AsmInterpretedFrame::GetFunctionOffset(false)), callTargetRegister);
    // get baseline code
    __ Movq(Operand(callTargetRegister, JSFunction::BASELINECODE_OFFSET), opRegister);
    Label baselineCodeUndefined;
    __ Cmpq(JSTaggedValue::Undefined().GetRawData(), opRegister);
    __ Je(&baselineCodeUndefined);
    // check is compiling
    __ Cmpq(JSTaggedValue::Hole().GetRawData(), opRegister);
    __ Je(&baselineCodeUndefined);
    {
        Register newSpRegister = r11; // r11 is free
        __ Leaq(Operand(rsp, AsmInterpretedFrame::GetSize(false)), newSpRegister);
        Label stackAligned;
        // align 16 bytes
        __ Testq(15, rsp);  // 15: low 4 bits must be 0b0000
        __ Jz(&stackAligned);
        __ PushAlignBytes();
        __ Bind(&stackAligned);

        Register bytecodePc = opRegister;
        // get bytecode pc
        __ Movq(Operand(frameStateBase, AsmInterpretedFrame::GetPcOffset(false)), bytecodePc);
        // get func
        Register func = callTargetRegister;
        // save glue
        __ Push(glueRegister);
        // save callTarget
        __ Push(callTargetRegister);
        // save new sp
        __ Push(newSpRegister);
        // callee save
        __ PushCppCalleeSaveRegisters();

        __ Movq(glueRegister, rax);
        // get native pc offset in baselinecode by bytecodePc in func
        __ Pushq(bytecodePc);
        __ Pushq(func); // argv[0]
        __ Pushq(2);    // 2: argc
        __ Pushq(kungfu::RuntimeStubCSigns::ID_GetNativePcOfstForBaseline);
        __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);

        __ Addq(4 * FRAME_SLOT_SIZE, rsp); // 4: skip runtimeId argc func bytecodePc

        __ PopCppCalleeSaveRegisters();
        __ Pop(newSpRegister);
        __ Pop(callTargetRegister);
        __ Pop(glueRegister);

        // restore glue, method, rbp
        __ Movq(glueRegister, r13);
        __ Movq(Operand(callTargetRegister, JSFunctionBase::METHOD_OFFSET), methodRegister);
        __ Movq(methodRegister, rbx);
        __ Movq(newSpRegister, rbp);

        // update pc
        const int32_t pcOffsetFromSp = -24; // -24: 3 slots, frameType, prevFrame, pc
        __ Movabs(std::numeric_limits<uint64_t>::max(), opRegister);
        __ Movq(opRegister, Operand(rbp, pcOffsetFromSp));

        // jmp to baselinecode
        __ Jmp(rax);
    }

    __ Bind(&baselineCodeUndefined);
    {
        // r13, rbp, r12, rbx,      r14,     rsi,  rdi
        // glue sp   pc  constpool  profile  acc   hotness
        __ Movq(Operand(frameStateBase, AsmInterpretedFrame::GetFunctionOffset(false)), callTargetRegister);
        __ Movq(Operand(frameStateBase, AsmInterpretedFrame::GetPcOffset(false)), r12);
        __ Movq(Operand(frameStateBase, AsmInterpretedFrame::GetAccOffset(false)), rsi);
        __ Movq(Operand(callTargetRegister, JSFunctionBase::METHOD_OFFSET), methodRegister);

        __ Leaq(Operand(rsp, AsmInterpretedFrame::GetSize(false)), opRegister);

        __ Cmpq(0, hasExceptionRegister);
        __ Jne(&gotoExceptionHandler);
        AsmInterpreterCall::DispatchCall(assembler, r12, opRegister, callTargetRegister, methodRegister, rsi, false);
        __ Bind(&gotoExceptionHandler);
        AsmInterpreterCall::DispatchCall(assembler, r12, opRegister, callTargetRegister, methodRegister, rsi, true);
    }

    __ Bind(&stackOverflow);
    {
        [[maybe_unused]] TempRegisterScope scope(assembler);
        Register temp = __ TempRegister();
        AsmInterpreterCall::ThrowStackOverflowExceptionAndReturnToAsmInterpBridgeFrame(assembler,
            glueRegister, rsp, temp);
    }
}


void OptimizedCall::DeoptPushAsmInterpBridgeFrame(ExtendedAssembler *assembler, Register context)
{
    Label processLazyDeopt;
    Label exit;
    Register frameTypeRegister = r8;
    __ Movq(Operand(context, AsmStackContext::GetIsFrameLazyDeoptOffset(false)), frameTypeRegister);
    __ Cmpq(0, frameTypeRegister);
    __ Jne(&processLazyDeopt);
    {
        __ Pushq(static_cast<int64_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME));
        __ Jmp(&exit);
    }
    __ Bind(&processLazyDeopt);
    {
        __ Pushq((static_cast<uint64_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME) |
                 (1ULL << FrameIterator::LAZY_DEOPT_FLAG_BIT)));
    }
    __ Bind(&exit);
    __ Pushq(rbp);
    __ Pushq(0);    // pc
    __ Leaq(Operand(rsp, 24), rbp);  // 24: skip pc, prevSp and frame type
    __ PushAlignBytes();
    if (!assembler->FromInterpreterHandler()) {
        __ PushCppCalleeSaveRegisters();
    }
}

// Input: %rdi - glue
// %rsi - deopt type
// %rdx - maybeAcc
void OptimizedCall::DeoptHandlerAsm(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(DeoptHandlerAsm));

    Register glueReg = rdi;
    PushAsmBridgeFrame(assembler);
    __ Push(glueReg);
    __ PushCppCalleeSaveRegisters();

    __ Movq(rdi, rax); // glue
    Register deoptType = rsi;
    Register maybeAcc = rdx;
    __ Subq(FRAME_SLOT_SIZE, rsp);
    __ Pushq(maybeAcc);   // acc
    __ Pushq(deoptType);  // argv[0]
    __ Pushq(2);          // 2: argc
    __ Pushq(kungfu::RuntimeStubCSigns::ID_DeoptHandler);
    __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);

    __ Addq(5 * FRAME_SLOT_SIZE, rsp); // 5: skip runtimeId argc deoptType maybeAcc align

    Register context = rsi;
    __ Movq(rax, context);

    Label target;
    __ PopCppCalleeSaveRegisters();
    __ Pop(glueReg);

    Label stackOverflow;
    __ Cmpq(JSTaggedValue::VALUE_EXCEPTION, rax);
    __ Je(&stackOverflow);

    __ Movq(Operand(context, AsmStackContext::GetCallerFpOffset(false)), rbp);
    __ Movq(Operand(context, AsmStackContext::GetCallFrameTopOffset(false)), rsp);
    __ Subq(FRAME_SLOT_SIZE, rsp); // skip lr

    DeoptPushAsmInterpBridgeFrame(assembler, context);
    __ Callq(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    DeoptEnterAsmInterpOrBaseline(assembler);
    __ Int3();

    __ Bind(&stackOverflow);
    {
        __ Movq(rdi, rax);
        __ Pushq(0); // argc
        __ Pushq(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException);
        __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
        __ Addq(FRAME_SLOT_SIZE * 3, rsp); // 3 : skip runtimeId argc & type
        __ Popq(rbp);
        __ Ret();
    }
}
#undef __
}  // namespace panda::ecmascript::x64
