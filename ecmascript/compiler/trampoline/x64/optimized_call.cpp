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

#include "ecmascript/compiler/trampoline/x64/common_call.h"

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/message_string.h"
#include "ecmascript/method.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript::x64 {
#define __ assembler->

// * uint64_t JSFunctionEntry(uintptr_t glue, uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t prevFp,
//                            size_t callType)
// * Arguments:
//        %rdi - glue
//        %rsi - actualNumArgs
//        %rdx - argV
//        %rcx - prevFp
//        %r8  - callType
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
    Register flag = r8;
    Label lJSCallNewWithArgV;
    Label lPopFrame;

    __ PushCppCalleeSaveRegisters();
    __ Pushq(glueReg); // caller save
    // construct the frame
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_ENTRY_FRAME));
    __ Pushq(prevFpReg);
    // 2: skip prevFp and frameType
    __ Leaq(Operand(rsp, 2 * FRAME_SLOT_SIZE), rbp);
    __ Movq(flag, r12);
    __ Movq(argv, rbx);
    __ Movq(Operand(rbx, 0), rdx);
    __ Movq(Operand(rbx, FRAME_SLOT_SIZE), rcx);
    __ Movq(Operand(rbx, DOUBLE_SLOT_SIZE), r8);
    __ Addq(TRIPLE_SLOT_SIZE, rbx);
    __ Movq(rbx, r9);
    __ Cmpl(1, r12);
    __ Je(&lJSCallNewWithArgV);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgV), false);
    __ Jmp(&lPopFrame);

    __ Bind(&lJSCallNewWithArgV);
    {
        __ CallAssemblerStub(RTSTUB_ID(JSCallNewWithArgV), false);
    }

    __ Bind(&lPopFrame);
    __ Popq(prevFpReg);
    __ Addq(FRAME_SLOT_SIZE, rsp); // 8: frame type
    __ Popq(rbp);
    __ Popq(glueReg); // caller restore
    __ PopCppCalleeSaveRegisters(); // callee restore
    __ Movq(prevFpReg, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Ret();
}

// * uint64_t OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs, uint32_t actualNumArgs,
//                                   uintptr_t codeAddr, uintptr_t argv, uintptr_t lexEnv)
// * Arguments wil CC calling convention:
//         %rdi - glue
//         %rsi - codeAddr
//         %rdx - actualNumArgs
//         %rcx - expectedNumArgs
//         %r8  - argv
//         %r9  - lexEnv
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
    Register glueReg = rdi;
    Register expectedNumArgsReg = rcx;
    Register actualNumArgsReg = rdx;
    Register codeAddrReg = rsi;
    Register argvReg = r8;
    Register envReg = r9;

    Label lAlign16Bytes1;
    Label lCopyExtraAument1;
    Label lCopyArguments1;
    Label lCopyLoop1;
    Label lPopFrame1;
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME));
    __ Pushq(envReg);
    // 2: skip envReg and frameType
    __ Leaq(Operand(rsp, 2 * FRAME_SLOT_SIZE), rbp);
    // callee save
    __ Pushq(r14);
    __ Pushq(rbx);
    __ Pushq(rax);

    // 16 bytes align check
    __ Movl(expectedNumArgsReg, r14);
    __ Testb(1, r14);
    __ Jne(&lAlign16Bytes1);
    __ Pushq(0);

    __ Bind(&lAlign16Bytes1);
    // expectedNumArgs > actualNumArgs
    __ Movl(expectedNumArgsReg, rbx);
    __ Cmpl(actualNumArgsReg, expectedNumArgsReg); // save expectedNumArgs
    __ Jbe(&lCopyArguments1);
    __ Movl(actualNumArgsReg, rax);
    __ Movl(rbx, expectedNumArgsReg);

    __ Bind(&lCopyExtraAument1); // copy undefined value to stack
    __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
    __ Addq(-1, expectedNumArgsReg);
    __ Cmpq(rax, expectedNumArgsReg);
    __ Ja(&lCopyExtraAument1);

    __ Bind(&lCopyArguments1);
    __ Cmpl(actualNumArgsReg, rbx);
    __ CMovbe(rbx, actualNumArgsReg);
    __ Movl(actualNumArgsReg, rax); // rax = actualNumArgsReg

    __ Bind(&lCopyLoop1);
    __ Movq(Operand(argvReg, rax, Scale::Times8, -FRAME_SLOT_SIZE), rbx); // -8: stack index
    __ Pushq(rbx);
    __ Addq(-1, rax);
    __ Jne(&lCopyLoop1);
    __ Pushq(actualNumArgsReg); // actual argc
    __ Pushq(envReg);

    __ Movq(glueReg, rax); // mov glue to rax
    __ Callq(codeAddrReg); // then call jsFunction
    __ Leaq(Operand(r14, Scale::Times8, 0), codeAddrReg);
    __ Addq(codeAddrReg, rsp);
    __ Addq(DOUBLE_SLOT_SIZE, rsp); // skip actualNumArgsReg and envReg
    __ Testb(1, r14); // stack 16bytes align check
    __ Jne(&lPopFrame1);
    __ Addq(8, rsp); // 8: align byte

    __ Bind(&lPopFrame1);
    __ Addq(8, rsp); // 8: skip rax
    __ Popq(rbx);
    __ Popq(r14);
    __ Addq(DOUBLE_SLOT_SIZE, rsp); // skip frame type, env reg
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
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_FROM_AOT);
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
    Register glueReg = rax;
    Register nativeCode = rsi;

    __ Movq(glueReg, Operand(rsp, FRAME_SLOT_SIZE)); // thread (instead of env)

    AsmInterpreterCall::PushBuiltinFrame(assembler, glueReg, FrameType::BUILTIN_CALL_LEAVE_FRAME);
    __ Leaq(Operand(rbp, 2 * FRAME_SLOT_SIZE), rdi); // 16: skip argc & env
    __ PushAlignBytes();
    AsmInterpreterCall::CallNativeInternal(assembler, nativeCode);
    __ Ret();
}

// * uint64_t JSProxyCallInternalWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, uintptr_t argv[])
// * c++ calling convention call js function
// * Arguments:
//        %rdi - glue
//        %rsi - argc
//        %rdx - calltarget
//        %rcx - argV[] = { calltarget, newtarget, thisObj, arg[0], arg[1], ..., arg[N-1])

void OptimizedCall::JSProxyCallInternalWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSProxyCallInternalWithArgV));
    Label jsCall;
    Label lJSCallStart;
    Label lNotJSFunction;
    Label lNonCallable;
    Label lJSFunctionCall;
    Label lJSBoundFunction;
    Label lJSProxy;
    Label lCallOptimziedMethod;
    Label lDirectCallCodeEntry;
    Label lCallNativeMethod;
    Label lAlign16Bytes2;
    Label lCopyBoundArgument;
    Label lCopyArgument2;
    Label lPushCallTarget;
    Label lCopyBoundArgumentLoop;
    Label lPopFrame2;
    Register glueReg = rdi;
    Register callTarget = rdx;
    Register argvReg = rcx;
    __ Movq(callTarget, Operand(argvReg, 0));
    __ Movq(callTarget, rax);
    __ Jmp(&lJSCallStart);
    __ Bind(&jsCall);
    {
        __ Movq(glueReg, rdi);
        glueReg = rdi;
        __ Movq(Operand(rsp, TRIPLE_SLOT_SIZE), rax); // get jsFunc
    }
    __ Bind(&lJSCallStart);
    Register jsFuncReg = rax;
    {
        __ Movabs(JSTaggedValue::TAG_INT, rdx); // IsTaggedInt
        __ And(jsFuncReg, rdx);
        __ Cmp(0x0, rdx);
        __ Jne(&lNonCallable);
        __ Cmp(0x0, jsFuncReg); // IsHole
        __ Je(&lNonCallable);
        __ Movabs(JSTaggedValue::TAG_SPECIAL, rdx);
        __ And(jsFuncReg, rdx);  // IsSpecial
        __ Cmp(0x0, rdx);
        __ Jne(&lNonCallable);

        __ Movq(jsFuncReg, rsi); // save jsFunc
        __ Movq(Operand(jsFuncReg, JSFunction::HCLASS_OFFSET), rax); // get jsHclass
        Register jsHclassReg = rax;
        __ Movl(Operand(jsHclassReg, JSHClass::BIT_FIELD_OFFSET), rax);
        __ Btl(JSHClass::CallableBit::START_BIT, rax); // IsCallable
        __ Jnb(&lNonCallable);

        __ Cmpb(static_cast<int32_t>(JSType::JS_FUNCTION_FIRST), rax);
        __ Jb(&lNotJSFunction);
        __ Cmpb(static_cast<int32_t>(JSType::JS_FUNCTION_LAST), rax);
        __ Jbe(&lJSFunctionCall);
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
        __ Addq(4 * FRAME_SLOT_SIZE, rsp); // 32: sp + 32 argv
        __ Pop(rbp);
        __ Ret();
    }

    __ Bind(&lJSFunctionCall);
    jsFuncReg = rsi;
    Register argc = r8;
    Register methodCallField = rcx;
    Register method = rdx;
    Register argV = r9;
    {
        __ Movq(Operand(jsFuncReg, JSFunction::LEXICAL_ENV_OFFSET), rdx);
        __ Movq(rdx, Operand(rsp, FRAME_SLOT_SIZE));
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
        __ Movl(Operand(rsp, DOUBLE_SLOT_SIZE), argc); // sp + 16 actual argc
        __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField); // get call field
        __ Btq(MethodLiteral::IsNativeBit::START_BIT, methodCallField); // is native
        __ Jb(&lCallNativeMethod);
        __ Btq(MethodLiteral::IsAotCodeBit::START_BIT, methodCallField); // is aot
        __ Jb(&lCallOptimziedMethod);
        __ Movq(rsp, argV);
        __ Addq(TRIPLE_SLOT_SIZE, argV); // sp + 24 get aot argv
        __ Subq(Immediate(kungfu::ArgumentAccessor::GetFixArgsNum()), argc);
        // argv + 24 get asm interpreter argv
        __ Addq(kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE, argV);
        OptimizedCallAsmInterpreter(assembler);
    }

    __ Bind(&lCallOptimziedMethod);
    Register codeAddrReg = rsi;
    Register expectedNumArgsReg = rcx;
    {
        __ Mov(Operand(method, Method::CODE_ENTRY_OFFSET), codeAddrReg); // get codeAddress
        __ Movq(argc, rdx);  // argc -> rdx
        __ Shr(MethodLiteral::NumArgsBits::START_BIT, methodCallField);
        __ Andl(((1LU <<  MethodLiteral::NumArgsBits::SIZE) - 1), methodCallField);
        __ Addl(NUM_MANDATORY_JSFUNC_ARGS, methodCallField); // add mandatory argument
        __ Movq(rsp, r8);
        Register envReg = r9;
        __ Movq(Operand(r8, FRAME_SLOT_SIZE), envReg); // get env
        argvReg = r8;
        __ Addq(TRIPLE_SLOT_SIZE, argvReg); // get argv
        __ Cmpl(expectedNumArgsReg, rdx); // expectedNumArgs <= actualNumArgs
        __ Jg(&lDirectCallCodeEntry);
        __ CallAssemblerStub(RTSTUB_ID(OptimizedCallOptimized), true);
    }

    __ Bind(&lDirectCallCodeEntry);
    {
        __ Movq(glueReg, rax); // rax = glue
        __ Jmp(codeAddrReg);
    }

    __ Bind(&lCallNativeMethod);
    {
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // Get MethodLiteral
        Register nativePointer = rsi;
        __ Mov(Operand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET), nativePointer); // native pointer
        __ Movq(glueReg, rax);
        CallBuiltinTrampoline(assembler);
    }

    __ Bind(&lJSBoundFunction);
    {
        __ Pushq(rbp);
        __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME));
        __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
        __ Pushq(r10); // callee save
        __ Movq(rsp, rdx);
        __ Addq(QUINTUPLE_SLOT_SIZE, rdx); // sp + 40 argv
        __ Mov(Operand(rdx, 0), rax); // get origin argc
        Register envReg = r9;
        __ Mov(Operand(rdx, -FRAME_SLOT_SIZE), envReg); // get env
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
    }

    __ Bind(&lAlign16Bytes2);
    {
        __ Subq(NUM_MANDATORY_JSFUNC_ARGS, rax);
        __ Cmp(0, rax);
        __ Je(&lCopyBoundArgument);
    }

    __ Bind(&lCopyArgument2);
    {
        __ Movq(Operand(rdx, rax, Scale::Times8, 3 * FRAME_SLOT_SIZE), rcx); // 24: slot size
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
        __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_THIS_OFFSET), rax); // thisObj
        __ Pushq(rax);
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED); // newTarget
        __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_TARGET_OFFSET), rax); // callTarget
        __ Pushq(rax);
        __ Pushq(r10); // push actual arguments
        Register envReg = r9;
        __ Pushq(envReg);
        __ Movq(rdi, rax);
        __ Callq(&jsCall); // call JSCall
        __ Addq(FRAME_SLOT_SIZE, rsp); // skip env
        __ Pop(r10);
        __ Leaq(Operand(r10, Scale::Times8, 0), rcx); // 8: offset
        __ Addq(rcx, rsp);
        __ Testb(1, r10);  // stack 16bytes align check
        __ Je(&lPopFrame2);
        __ Addq(FRAME_SLOT_SIZE, rsp); // 8: sp + 8
    }

    __ Bind(&lPopFrame2);
    {
        __ Pop(r10);
        __ Addq(FRAME_SLOT_SIZE, rsp); // 8: sp + 8
        __ Pop(rbp);
        __ Ret();
    }
    __ Bind(&lJSProxy);
    __ Movq(rsp, rcx);
    __ Movq(jsFuncReg, rdx);
    __ Addq(DOUBLE_SLOT_SIZE, rcx); // skip returnAddr
    __ Mov(Operand(rcx, 0), rsi); // get origin argc
    __ Addq(FRAME_SLOT_SIZE, rcx); // 8: sp + 8 argv
    __ Movq(kungfu::CommonStubCSigns::JsProxyCallInternal, r9);
    __ Movq(Operand(rdi, r9, Scale::Times8, JSThread::GlueData::GetCOStubEntriesOffset(false)), r8);
    __ Jmp(r8);
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
    Label lCallOptimziedMethod;
    Label lCallNativeMethod;
    Label lCallNativeCpp;
    Label lCallNativeBuiltinStub;
    Register glueReg = rax;
    __ Bind(&jsCall);
    {
        __ Movq(glueReg, rdi);
        glueReg = rdi;
        __ Movq(Operand(rsp, TRIPLE_SLOT_SIZE), rax); // sp + 24 get jsFunc
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
        __ Movq(Operand(jsFuncReg, JSFunction::LEXICAL_ENV_OFFSET), rdx);
        __ Movq(rdx, Operand(rsp, FRAME_SLOT_SIZE));
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
        __ Movl(Operand(rsp, DOUBLE_SLOT_SIZE), argc); // sp + 16 actual argc
        __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField); // get call field
        __ Btq(MethodLiteral::IsNativeBit::START_BIT, methodCallField); // is native
        __ Jb(&lCallNativeMethod);
        if (!isNew) {
            __ Btq(JSHClass::ClassConstructorBit::START_BIT, rax); // is CallConstructor
            __ Jb(&lCallConstructor);
        }
        __ Btq(MethodLiteral::IsAotCodeBit::START_BIT, methodCallField); // is aot
        __ Jb(&lCallOptimziedMethod);
        __ Movq(rsp, argV);
        __ Addq(TRIPLE_SLOT_SIZE, argV); // sp + 24 get aot argv
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
            __ Addq(4 * FRAME_SLOT_SIZE, rsp);
            __ Pop(rbp);
            __ Ret();
        }
    }

    __ Bind(&lCallOptimziedMethod);
    Register codeAddrReg = rsi;
    Register expectedNumArgsReg = rcx;
    {
        CallOptimziedMethodInternal(assembler, glueReg, jsFuncReg,
                                    methodCallField, argc, codeAddrReg, expectedNumArgsReg);
    }

    __ Bind(&lCallNativeMethod);
    {
        Register nativePointer = rsi;
        method = rax;
        __ Movq(jsFuncReg, rdx);
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method);  // get method
        __ Mov(Operand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET), nativePointer);  // native pointer
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
        CallBuiltinTrampoline(assembler);
    }

    __ Bind(&lCallNativeBuiltinStub);
    {
        Register methodExtraLiteralInfo = rax;
        __ Mov(Operand(method, Method::EXTRA_LITERAL_INFO_OFFSET), methodExtraLiteralInfo);  // get extra literal
        __ Shr(MethodLiteral::BuiltinIdBits::START_BIT, methodExtraLiteralInfo);
        __ Andl(((1LU <<  MethodLiteral::BuiltinIdBits::SIZE) - 1), methodExtraLiteralInfo);  // get builtin stub id
        if (!isNew) {
            __ Cmpl(kungfu::BuiltinsStubCSigns::BUILTINS_CONSTRUCTOR_STUB_FIRST, methodExtraLiteralInfo);
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
        Label lexit;
        argV = rax;

        __ Movq(rsp, argV);
        __ Addq(SEXTUPLE_SLOT_SIZE, argV);
        __ Pushq(rbp);
        __ Pushq(static_cast<int32_t>(FrameType::ASM_BRIDGE_FRAME));
        __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);

        if (!isNew) {
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
                __ PushAlignBytes();
                __ Callq(r10);
                __ Addq(DOUBLE_SLOT_SIZE, rsp);
                __ Jmp(&lexit);
            }

            __ Bind(&lCall1);
            {
                __ Movq(Operand(argV, 0), r11);                     // arg0
                __ Pushq(r11);
                __ Callq(r10);
                __ Addq(DOUBLE_SLOT_SIZE, rsp);
                __ Jmp(&lexit);
            }

            __ Bind(&lCall2);
            {
                __ PushAlignBytes();
                __ Movq(Operand(argV, FRAME_SLOT_SIZE), r11);        // arg1
                __ Pushq(r11);
                __ Movq(Operand(argV, 0), r11);                      // arg0
                __ Pushq(r11);
                __ Callq(r10);
                __ Addq(QUADRUPLE_SLOT_SIZE, rsp);
                __ Jmp(&lexit);
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
            }
        } else {
            __ Pushq(argV);                                        // argv
            __ Callq(r10);
            __ Addq(DOUBLE_SLOT_SIZE, rsp);
        }

        __ Bind(&lexit);
        {
            __ Pop(rbp);
            __ Ret();
        }
    }

    __ Bind(&lJSBoundFunction);
    {
        JSBoundFunctionCallInternal(assembler, jsFuncReg, &jsCall);
    }
    __ Bind(&lJSProxy);
    JSProxyCallInternal(assembler, jsFuncReg);
}

void OptimizedCall::ConstructorJSCall(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ConstructorJSCall));
    Label constructorJsCall;
    Label lConstructorJSCallStart;
    Label lNotJSFunction;
    Label lNonCallable;
    Label lJSFunctionCall;
    Label lJSBoundFunction;
    Label lJSProxy;
    Label lCallOptimziedMethod;
    Label lCallNativeMethod;
    Register glueReg = rax;
    __ Bind(&constructorJsCall);
    {
        __ Movq(glueReg, rdi);
        glueReg = rdi;
        __ Movq(Operand(rsp, TRIPLE_SLOT_SIZE), rax); // sp + 24 get jsFunc
    }
    __ Bind(&lConstructorJSCallStart);
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
        __ Movq(Operand(jsFuncReg, JSFunction::LEXICAL_ENV_OFFSET), rdx);
        __ Movq(rdx, Operand(rsp, FRAME_SLOT_SIZE));
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
        __ Movl(Operand(rsp, DOUBLE_SLOT_SIZE), argc); // sp + 16 actual argc
        __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField); // get call field
        __ Btq(MethodLiteral::IsNativeBit::START_BIT, methodCallField); // is native
        __ Jb(&lCallNativeMethod);
        __ Btq(MethodLiteral::IsAotCodeBit::START_BIT, methodCallField); // is aot
        __ Jb(&lCallOptimziedMethod);
        __ Movq(rsp, argV);
        __ Addq(TRIPLE_SLOT_SIZE, argV); // sp + 24 get aot argv
        // argv + 24 get asm interpreter argv
        __ Addq(kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE, argV);
        OptimizedCallAsmInterpreter(assembler);
    }

    __ Bind(&lCallOptimziedMethod);
    Register codeAddrReg = rsi;
    Register expectedNumArgsReg = rcx;
    {
        CallOptimziedMethodInternal(assembler, glueReg, jsFuncReg,
                                    methodCallField, argc, codeAddrReg, expectedNumArgsReg);
    }

    __ Bind(&lCallNativeMethod);
    {
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // Get MethodLiteral
        Register nativePointer = rsi;
        __ Mov(Operand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET), nativePointer); // native pointer
        __ Movq(glueReg, rax);
        CallBuiltinTrampoline(assembler);
    }

    __ Bind(&lJSBoundFunction);
    {
        JSBoundFunctionCallInternal(assembler, jsFuncReg, &constructorJsCall);
    }
    __ Bind(&lJSProxy);
    JSProxyCallInternal(assembler, jsFuncReg);
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
    __ Addq(4 * FRAME_SLOT_SIZE, rsp); // 32: sp + 32 argv
    __ Pop(rbp);
    __ Ret();
}

void OptimizedCall::CallOptimziedMethodInternal(ExtendedAssembler *assembler, Register glueReg, Register jsFuncReg,
                                                Register methodCallField,  Register argc,
                                                Register codeAddrReg, Register expectedNumArgsReg)
{
    Label lDirectCallCodeEntry;
    Register method = rdx;
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Mov(Operand(method, Method::CODE_ENTRY_OFFSET), codeAddrReg); // get codeAddress
    __ Movq(argc, rdx);  // argc -> rdx
    __ Shr(MethodLiteral::NumArgsBits::START_BIT, methodCallField);
    __ Andl(((1LU <<  MethodLiteral::NumArgsBits::SIZE) - 1), methodCallField);
    __ Addl(NUM_MANDATORY_JSFUNC_ARGS, methodCallField); // add mandatory argumentr
    __ Movq(rsp, r8);
    Register envReg = r9;
    __ Movq(Operand(r8, FRAME_SLOT_SIZE), envReg); // get env
    Register argvReg = r8;
    __ Addq(3 * FRAME_SLOT_SIZE, argvReg); // 3 : sp + 3 * 8 argv
    __ Cmpl(expectedNumArgsReg, rdx); // expectedNumArgs <= actualNumArgs
    __ Jge(&lDirectCallCodeEntry);
    __ CallAssemblerStub(RTSTUB_ID(OptimizedCallOptimized), true);
    __ Bind(&lDirectCallCodeEntry);
    {
        __ Movq(glueReg, rax); // rax = glue
        __ Jmp(codeAddrReg);
    }
}

void OptimizedCall::JSBoundFunctionCallInternal(ExtendedAssembler *assembler, Register jsFuncReg, Label *jsCall)
{
    Label lAlign16Bytes2;
    Label lCopyBoundArgument;
    Label lCopyArgument2;
    Label lPushCallTarget;
    Label lCopyBoundArgumentLoop;
    Label lPopFrame2;
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
    __ Pushq(r10); // callee save
    __ Movq(rsp, rdx);
    __ Addq(QUINTUPLE_SLOT_SIZE, rdx); // sp + 40 argv
    __ Mov(Operand(rdx, 0), rax); // get origin argc
    Register envReg = r9;
    __ Mov(Operand(rdx, -FRAME_SLOT_SIZE), envReg); // get env
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
        __ Movq(Operand(rdx, rax, Scale::Times8, 3 * FRAME_SLOT_SIZE), rcx); // 24 : disp
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
        __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_THIS_OFFSET), rax); // thisObj
        __ Pushq(rax);
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED); // newTarget
        __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_TARGET_OFFSET), rax); // callTarget
        __ Pushq(rax);
        __ Pushq(r10); // push actual arguments
        envReg = r9;
        __ Pushq(envReg);
        __ Movq(rdi, rax);
        __ Callq(jsCall); // call JSCall
        __ Addq(8, rsp); // 8: sp + 8
        __ Pop(r10);
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

void OptimizedCall::JSProxyCallInternal(ExtendedAssembler *assembler, Register jsFuncReg)
{
    __ Movq(jsFuncReg, rdx); // calltarget
    __ Movq(rsp, rcx);
    __ Addq(DOUBLE_SLOT_SIZE, rcx); // sp + 16 skip returnAddr
    __ Mov(Operand(rcx, 0), rsi); // get origin argc
    __ Addq(FRAME_SLOT_SIZE, rcx); // 8: sp + 8 argv
    __ Movq(kungfu::CommonStubCSigns::JsProxyCallInternal, r9);
    __ Movq(Operand(rdi, r9, Scale::Times8, JSThread::GlueData::GetCOStubEntriesOffset(false)), r8);
    __ Jmp(r8);
    __ Ret();
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
    // 16: rbp & return address
    __ Addq(2 * FRAME_SLOT_SIZE, rdx);

    __ Movq(Operand(rdx, 0), r10);
    __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
    __ Movq(rax, rdi);
    // 8: argc
    __ Movq(Operand(rdx, FRAME_SLOT_SIZE), rsi);
    // 16: argv
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
    __ Addq(1, expectedNumArgs);
    __ Andq(~1, expectedNumArgs);
    __ Leaq(Operand(expectedNumArgs, Scale::Times8, 0), expectedNumArgs);
    __ Addq(expectedNumArgs, rsp);
    __ Addq(FRAME_SLOT_SIZE, rsp); // 8: skip expectedNumArgs
}

void OptimizedCall::PushJSFunctionEntryFrame (ExtendedAssembler *assembler, Register prevFp)
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
    // 16 : 16 means pop call site sp and type
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

void OptimizedCall::GenJSCallWithArgV(ExtendedAssembler *assembler, bool isNew)
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
    __ Pushq(actualNumArgs);
    __ Movq(Operand(jsfunc, JSFunction::LEXICAL_ENV_OFFSET), rax);
    __ Pushq(rax);
    __ Movq(glue, rax);
    if (isNew) {
        __ CallAssemblerStub(RTSTUB_ID(JSCallNew), false);
    } else {
        __ CallAssemblerStub(RTSTUB_ID(JSCall), false);
    }
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Mov(Operand(sp, 0), actualNumArgs);
    PopJSFunctionArgs(assembler, actualNumArgs);
    PopOptimizedUnfoldArgVFrame(assembler);
    __ Ret();
}

void OptimizedCall::JSCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallWithArgV));
    GenJSCallWithArgV(assembler, false);
}

void OptimizedCall::JSCallNewWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallNewWithArgV));
    GenJSCallWithArgV(assembler, true);
}

void OptimizedCall::ConstructorJSCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ConstructorJSCallWithArgV));
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
    __ Pushq(actualNumArgs);
    __ Movq(Operand(jsfunc, JSFunction::LEXICAL_ENV_OFFSET), rax);
    __ Pushq(rax);
    __ Movq(glue, rax);
    __ CallAssemblerStub(RTSTUB_ID(ConstructorJSCall), false);
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Mov(Operand(sp, 0), actualNumArgs);
    PopJSFunctionArgs(assembler, actualNumArgs);
    PopOptimizedUnfoldArgVFrame(assembler);
    __ Ret();
}

// Input: %rdi - glue
//        %rsi - context
void OptimizedCall::DeoptEnterAsmInterp(ExtendedAssembler *assembler)
{
    // rdi
    Register glueRegister = __ GlueRegister();
    Register context = rsi;
    // rax, rdx, rcx, r8, r9, r10, r11 is free
    Register tempRegister = rax;
    Register opRegister = r10;
    Register outputCount = rdx;
    Register frameStateBase = rcx;
    __ Movq(Operand(context, AsmStackContext::GetOutputCountOffset(false)), outputCount);
    __ Leaq(Operand(context, AsmStackContext::GetSize(false)), frameStateBase);

    Label stackOverflow;
    // update fp
    __ Movq(rsp, Operand(frameStateBase, AsmInterpretedFrame::GetFpOffset(false)));
    PushArgsWithArgvAndCheckStack(assembler, glueRegister, outputCount,
        frameStateBase, tempRegister, opRegister, &stackOverflow);

    Register callTargetRegister = r8;
    Register methodRegister = r9;
    {
        // r13, rbp, r12, rbx,      r14,     rsi,  rdi
        // glue sp   pc  constpool  profile  acc   hotness
        __ Movq(Operand(frameStateBase, AsmInterpretedFrame::GetFunctionOffset(false)), callTargetRegister);
        __ Movq(Operand(frameStateBase, AsmInterpretedFrame::GetPcOffset(false)), r12);
        __ Movq(Operand(frameStateBase, AsmInterpretedFrame::GetAccOffset(false)), rsi);
        __ Movq(Operand(callTargetRegister, JSFunctionBase::METHOD_OFFSET), methodRegister);

        __ Leaq(Operand(rsp, AsmInterpretedFrame::GetSize(false)), opRegister);
        AsmInterpreterCall::DispatchCall(assembler, r12, opRegister, methodRegister, rsi);
    }

    __ Bind(&stackOverflow);
    {
        [[maybe_unused]] TempRegisterScope scope(assembler);
        Register temp = __ TempRegister();
        AsmInterpreterCall::ThrowStackOverflowExceptionAndReturn(assembler,
            glueRegister, rsp, temp);
    }
}

// Input: %rdi - glue
void OptimizedCall::DeoptHandlerAsm(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(DeoptHandlerAsm));

    Register glueReg = rdi;
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::ASM_BRIDGE_FRAME));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
    __ Push(glueReg);
    __ PushCppCalleeSaveRegisters();

    __ Movq(rdi, rax); // glue
    Register deoptType = rsi;
    __ Pushq(deoptType);  // argv[0]
    __ Pushq(1);  // argc
    __ Pushq(kungfu::RuntimeStubCSigns::ID_DeoptHandler);
    __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);

    __ Addq(3 * FRAME_SLOT_SIZE, rsp); // 3: skip runtimeId argc deoptType

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

    PushAsmInterpBridgeFrame(assembler);
    __ Callq(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    DeoptEnterAsmInterp(assembler);
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