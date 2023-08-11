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

// * uint64_t JSFunctionEntry(uintptr_t glue, uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t prevFp)
// * Arguments:
//        %rdi - glue
//        %rsi - actualNumArgs
//        %rdx - argV
//        %rcx - prevFp
//        %r8 - needPushUndefined
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
    Register needPushUndefined = r8;
    Label lJSCallWithArgVAndPushUndefined;
    Label lPopFrame;
    PushJSFunctionEntryFrame(assembler, prevFpReg);
    __ Movq(argv, rbx);
    __ Movq(needPushUndefined, r12);
    __ Movq(Operand(rbx, 0), rdx);
    __ Movq(Operand(rbx, FRAME_SLOT_SIZE), rcx);
    __ Movq(Operand(rbx, DOUBLE_SLOT_SIZE), r8);
    __ Addq(TRIPLE_SLOT_SIZE, rbx);
    __ Movq(rbx, r9);
    __ Cmp(1, r12);
    __ Je(&lJSCallWithArgVAndPushUndefined);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgV), false);
    __ Jmp(&lPopFrame);

    __ Bind(&lJSCallWithArgVAndPushUndefined);
    __ CallAssemblerStub(RTSTUB_ID(JSCallWithArgVAndPushUndefined), false);

    __ Bind(&lPopFrame);
    __ Popq(prevFpReg);
    __ Addq(FRAME_SLOT_SIZE, rsp); // 8: frame type
    __ Popq(rbp);
    __ Popq(glueReg); // caller restore
    __ PopCppCalleeSaveRegisters(); // callee restore
    __ Movq(prevFpReg, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Ret();
}

// * uint64_t OptimizedCallAndPushUndefined(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, JSTaggedType new,
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
//               |--------------------------| ---------------
//               |       returnAddr         |               ^
//      sp ----> |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|   OptimizedJSFunctionFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------
void OptimizedCall::OptimizedCallAndPushUndefined(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedCallAndPushUndefined));
    Register jsFuncReg = rdi;
    Register method = r9;
    Register codeAddrReg = rsi;
    __ Movq(Operand(rsp, DOUBLE_SLOT_SIZE), jsFuncReg); // sp + 16 get jsFunc
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Mov(Operand(method, Method::CODE_ENTRY_OFFSET), codeAddrReg);

    Register methodCallField = rcx;
    __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField); // get call field
    __ Shr(MethodLiteral::NumArgsBits::START_BIT, methodCallField);
    __ Andl(((1LU <<  MethodLiteral::NumArgsBits::SIZE) - 1), methodCallField);
    __ Addl(NUM_MANDATORY_JSFUNC_ARGS, methodCallField); // add mandatory argumentr

    __ Movl(Operand(rsp, FRAME_SLOT_SIZE), rdx); // argc rdx
    __ Movq(rsp, r8);
    Register argvReg = r8;
    auto funcSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;  // 1: return addr
    __ Addq(funcSlotOffset * FRAME_SLOT_SIZE, argvReg); // skip return addr and argc

    Register expectedNumArgsReg = rcx;
    Register actualNumArgsReg = rdx;

    Label lCopyExtraAument1;
    Label lCopyLoop1;
    Label lPopFrame1;
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
    // callee save
    __ Pushq(r14);
    __ Pushq(rbx);
    __ Pushq(rax);

    // 16 bytes align check
    __ Movl(expectedNumArgsReg, r14);
    __ Testb(1, r14);
    __ Jne(&lCopyExtraAument1);
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
    __ Pushq(actualNumArgsReg); // actual argc

    __ Callq(codeAddrReg); // then call jsFunction
    __ Leaq(Operand(r14, Scale::Times8, 0), codeAddrReg);
    __ Addq(codeAddrReg, rsp);
    __ Addq(FRAME_SLOT_SIZE, rsp); // skip actualNumArgsReg
    __ Testb(1, r14); // stack 16bytes align check
    __ Jne(&lPopFrame1);
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

void OptimizedCall::CallBuiltinTrampoline(ExtendedAssembler *assembler)
{
    Register glueReg = rax;
    Register nativeCode = rsi;

    __ Movq(Operand(rsp, 0), rdx);
    __ Movq(glueReg, Operand(rsp, 0));
    __ Push(rdx);

    AsmInterpreterCall::PushBuiltinFrame(assembler, glueReg, FrameType::BUILTIN_CALL_LEAVE_FRAME);
    __ Leaq(Operand(rbp, 2 * FRAME_SLOT_SIZE), rdi); // 2: skip rbp & return Addr
    AsmInterpreterCall::CallNativeInternal(assembler, nativeCode);
    __ Pop(rdx);
    __ Movq(rdx, Operand(rsp, 0));
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
    __ Movq(callTarget, Operand(rsp, DOUBLE_SLOT_SIZE));  // update callTarget slot
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
        __ Movq(Operand(rsp, DOUBLE_SLOT_SIZE), rax); // sp + 16 get jsFunc
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
        __ Addq(argvSlotOffset * FRAME_SLOT_SIZE, argV); // skip return addr and argc
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
        __ Movq(Operand(rsp, TRIPLE_SLOT_SIZE), rcx);              // newTarget
        __ Movq(Operand(rsp, QUADRUPLE_SLOT_SIZE), r8);               // this
        __ Subq(NUM_MANDATORY_JSFUNC_ARGS, r9);                       // argc

        Label lCall0;
        Label lCall1;
        Label lCall2;
        Label lCall3;
        Label lexit;
        argV = rax;

        __ Movq(rsp, argV);
        auto argvSlotOffset = kungfu::ArgumentAccessor::GetFixArgsNum() +
            kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;  // 1: return addr
        __ Addq(argvSlotOffset *FRAME_SLOT_SIZE, argV);
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
    __ Movq(rax, r10);
    // get bound target
    __ Mov(Operand(jsFuncReg, JSBoundFunction::BOUND_ARGUMENTS_OFFSET), rcx);
    // get bound length
    __ Mov(Operand(rcx, TaggedArray::LENGTH_OFFSET), rcx);
    __ Addq(rcx, r10);

    // 16 bytes align check
    __ Testb(1, r10);
    __ Jne(&lAlign16Bytes2);
    __ PushAlignBytes(); // push zero to align 16 bytes stack

    __ Bind(&lAlign16Bytes2);
    {
        __ Subq(NUM_MANDATORY_JSFUNC_ARGS, rax);
        __ Cmp(0, rax);
        __ Je(&lCopyBoundArgument);
    }

    __ Bind(&lCopyArgument2);
    {
        __ Movq(Operand(rdx, rax, Scale::Times8, kungfu::ArgumentAccessor::GetFixArgsNum() * FRAME_SLOT_SIZE), rcx);
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
        __ Pushq(r10); // push actual arguments
    }
    JSCallCheck(assembler, rax, &slowCall, &slowCall, &isJsFunc); // jsfunc -> rsi hclassfiled -> rax
    __ Jmp(&slowCall);
    Register jsfunc = rsi;
    __ Bind(&isJsFunc);
    {
        __ Btq(JSHClass::IsClassConstructorOrPrototypeBit::START_BIT, rax); // is CallConstructor
        __ Jnb(&isNotClass);
        __ Btq(JSHClass::ConstructorBit::START_BIT, rax);
        __ Jb(&slowCall);
        __ Bind(&isNotClass);
        __ Btq(JSHClass::IsOptimizedBit::START_BIT, rax); // is aot
        __ Jnb(&slowCall);
        __ Bind(&aotCall);
        {
            // output: glue:rdi argc:rsi calltarget:rdx argv:rcx this:r8 newtarget:r9
            __ Movq(jsfunc, rdx);
            __ Movq(r10, rsi);
            __ Leaq(Operand(rsp, 4 * FRAME_SLOT_SIZE), rcx); // 4: skip argc and func new this
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
        __ Leaq(Operand(r10, Scale::Times8, 0), rcx); // 8: disp
        __ Addq(rcx, rsp);
        __ Testb(1, r10);  // stack 16bytes align check
        __ Jne(&lPopFrame2);
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
    __ Addq(FRAME_SLOT_SIZE, rcx); // skip returnAddr
    __ Mov(Operand(rcx, 0), rsi); // get origin argc
    __ Addq(kungfu::ArgumentAccessor::GetExtraArgsNum() * FRAME_SLOT_SIZE, rcx); // skip extra args: argc
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
    Label align16Bytes;
    __ Testb(1, expectedNumArgs);
    __ Jne(&align16Bytes);
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Bind(&align16Bytes);
    __ Leaq(Operand(expectedNumArgs, Scale::Times8, 0), expectedNumArgs);
    __ Addq(expectedNumArgs, rsp);
    __ Addq(FRAME_SLOT_SIZE, rsp); // 8: skip expectedNumArgs
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

void OptimizedCall::GenJSCallWithArgV(ExtendedAssembler *assembler, bool needAddExpectedArgs)
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
    __ Je(&align16Bytes);
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
    __ Movq(glue, rax);
    if (needAddExpectedArgs) {
        __ CallAssemblerStub(RTSTUB_ID(OptimizedCallAndPushUndefined), false);
    } else {
        __ CallAssemblerStub(RTSTUB_ID(CallOptimized), false);
    }

    __ Mov(Operand(sp, 0), actualNumArgs);
    PopJSFunctionArgs(assembler, actualNumArgs);
    PopOptimizedUnfoldArgVFrame(assembler);
    __ Ret();
}

// * uint64_t JSCallWithArgVAndPushUndefined(uintptr_t glue, uint32_t argc, JSTaggedType calltarget,
//                          JSTaggedType new, JSTaggedType this, argV)
// * cc calling convention call js function()
// * arguments:
//              %rdi - glue
//              %rsi - argc
//              %rdx - call-target
//              %rcx - new-target
//              %r8  - this
//              %r9  - argv
void OptimizedCall::JSCallWithArgVAndPushUndefined(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallWithArgVAndPushUndefined));
    GenJSCallWithArgV(assembler, true);
}

void OptimizedCall::JSCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallWithArgV));
    GenJSCallWithArgV(assembler, false);
}

void OptimizedCall::CallOptimized(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallOptimized));
    Register jsFuncReg = rdi;
    Register method = r9;
    Register codeAddrReg = rsi;
    __ Movq(Operand(rsp, DOUBLE_SLOT_SIZE), jsFuncReg); // sp + 16 get jsFunc
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Mov(Operand(method, Method::CODE_ENTRY_OFFSET), codeAddrReg);
    __ Jmp(codeAddrReg);
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
    Register depth = r11;
    Label loopBegin;
    Label stackOverflow;
    Label pushArgv;
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
    Register depth = rdx;
    __ Subq(FRAME_SLOT_SIZE, rsp);
    __ Pushq(depth);
    __ Pushq(deoptType);  // argv[0]
    __ Pushq(2);  // 2: argc
    __ Pushq(kungfu::RuntimeStubCSigns::ID_DeoptHandler);
    __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);

    __ Addq(5 * FRAME_SLOT_SIZE, rsp); // 5: skip runtimeId argc deoptType depth align

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
