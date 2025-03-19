/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/message_string.h"

namespace panda::ecmascript::x64 {
#define __ assembler->

// * uint64_t OptimizedFastCallEntry(uintptr_t glue, uint32_t actualNumArgs, const JSTaggedType argV[],
//                                   uintptr_t prevFp)
// * Arguments:
//        %rdi - glue
//        %rsi - actualNumArgs
//        %rdx - argV
//        %rcx - prevFp

void OptimizedFastCall::OptimizedFastCallEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedFastCallEntry));
    Register glueReg = rdi;
    Register argv = rdx;
    Register prevFpReg = rcx;

    OptimizedCall::PushJSFunctionEntryFrame(assembler, prevFpReg);
    __ Movq(argv, r8);
    __ Movq(rsi, rcx);
    __ Movq(Operand(r8, 0), rsi); // func
    __ Movq(Operand(r8, FRAME_SLOT_SIZE), rdx); // thisobj
    __ Addq(DOUBLE_SLOT_SIZE, r8);
    __ CallAssemblerStub(RTSTUB_ID(JSFastCallWithArgV), false);

    __ Popq(prevFpReg);
    __ Addq(FRAME_SLOT_SIZE, rsp); // 8: frame type
    __ Popq(rbp);
    __ Popq(glueReg); // caller restore
    __ PopCppCalleeSaveRegisters(); // callee restore
    __ Movq(prevFpReg, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Ret();
}


// * uint64_t OptimizedFastCallAndPushArgv(uintptr_t glue, uint32_t expectedNumArgs, uint32_t actualNumArgs,
//                                   uintptr_t codeAddr, uintptr_t argv)
// * Arguments wil CC calling convention:
//         %rdi - glue
//         %rsi - actualNumArgs
//         %rdx - actualArgv
//         %rcx - func
//         %r8  - new target
//         %r9  - this
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
void OptimizedFastCall::OptimizedFastCallAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedFastCallAndPushArgv));
    Register actualNumArgsReg = rsi;
    Register jsFuncReg = rcx;
    Register thisObj = r9;
    Label lCopyExtraAument1;
    Label lCopyExtraUndefineToSp;
    Label lCopyLoop1;
    Label lCopyLoop2;
    Label pushUndefined;
    Label call;
    Label arg4;
    Label argc;
    Label checkExpectedArgs;
    JsFunctionArgsConfigFrameScope scope(assembler); // push frametype and callee save
    __ Movq(actualNumArgsReg, r13);
    actualNumArgsReg = r13;
    __ Movq(rcx, rsi); // func move to argc
    jsFuncReg = rsi;
    __ Movq(thisObj, rdx); // this move to argv
    Register method = r14;
    Register methodCallField = rbx;
    Register codeAddrReg = rax;
    Register argvReg = r12;
    __ Leaq(Operand(rsp, 8 * FRAME_SLOT_SIZE), argvReg); // 8: skip 8 frames to get argv
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Mov(Operand(jsFuncReg, JSFunctionBase::CODE_ENTRY_OFFSET), codeAddrReg); // get codeAddress
    __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField); // get call field
    __ Shr(MethodLiteral::NumArgsBits::START_BIT, methodCallField);
    __ Andl(((1LU <<  MethodLiteral::NumArgsBits::SIZE) - 1), methodCallField);
    __ Addl(NUM_MANDATORY_JSFUNC_ARGS, methodCallField); // add mandatory argumentr
    Register expectedNumArgsReg = rbx;

    Label arg5;
    Label arg6;
    __ Cmp(Immediate(3), actualNumArgsReg); // 3: func new this
    __ Jne(&arg4);
    __ Movq(JSTaggedValue::VALUE_UNDEFINED, rcx);
    __ Movq(JSTaggedValue::VALUE_UNDEFINED, r8);
    __ Movq(JSTaggedValue::VALUE_UNDEFINED, r9);
    __ Subq(3, expectedNumArgsReg); // 3: skip 3 register
    __ Jmp(&checkExpectedArgs);

    __ Bind(&arg4);
    {
        __ Movq(Operand(argvReg, 0), rcx);
        __ Addq(FRAME_SLOT_SIZE, argvReg);
        __ Cmp(Immediate(4), actualNumArgsReg); // 4: func new this arg0
        __ Jne(&arg5);
        __ Movq(JSTaggedValue::VALUE_UNDEFINED, r8);
        __ Movq(JSTaggedValue::VALUE_UNDEFINED, r9);
        __ Subq(3, expectedNumArgsReg); // 3: skip 3 register
        __ Jmp(&checkExpectedArgs);
    }

    __ Bind(&arg5);
    {
        __ Movq(Operand(argvReg, 0), r8);
        __ Addq(FRAME_SLOT_SIZE, argvReg);
        __ Cmp(Immediate(5), actualNumArgsReg); // 5: 5 args
        __ Jne(&arg6);
        __ Movq(JSTaggedValue::VALUE_UNDEFINED, r9);
        __ Subq(3, expectedNumArgsReg); // 3: skip 3 register
        __ Jmp(&checkExpectedArgs);
    }

    __ Bind(&arg6);
    {
        __ Movq(Operand(argvReg, 0), r9);
        __ Addq(FRAME_SLOT_SIZE, argvReg);
        __ Cmp(Immediate(6), actualNumArgsReg); // 6: 6 args
        __ Jne(&argc);
        __ Subq(3, expectedNumArgsReg); // 3: skip above 3 args
        __ Jmp(&checkExpectedArgs);
    }

    __ Bind(&argc); // actualNumArgsReg >=7
    {
        __ Cmpq(expectedNumArgsReg, actualNumArgsReg);
        __ Jb(&pushUndefined);
        // 16 bytes align check
        __ Subq(6, actualNumArgsReg); // 6: skip above 6 args
        __ Subq(6, expectedNumArgsReg); // 6: skip above 6 args
        __ Testb(1, actualNumArgsReg);
        __ Je(&lCopyLoop2);
        __ Pushq(0);
        __ Bind(&lCopyLoop2);
        __ Movq(Operand(argvReg, actualNumArgsReg, Scale::Times8, -FRAME_SLOT_SIZE), r14); // -8: stack index
        __ Pushq(r14);
        __ Subq(1, actualNumArgsReg);
        __ Jne(&lCopyLoop2);
        __ Jmp(&call);

        __ Bind(&pushUndefined);
        // 16 bytes align check
        __ Subq(6, actualNumArgsReg); // 6: skip above 6 args
        __ Subq(6, expectedNumArgsReg); // 6: skip above 6 args
        __ Testb(1, expectedNumArgsReg);
        __ Je(&lCopyExtraAument1);
        __ Pushq(0);
        __ Bind(&lCopyExtraAument1); // copy undefined value to stack
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
        __ Subq(1, expectedNumArgsReg);
        __ Cmpq(actualNumArgsReg, expectedNumArgsReg);
        __ Ja(&lCopyExtraAument1);
        __ Bind(&lCopyLoop1);
        __ Movq(Operand(argvReg, expectedNumArgsReg, Scale::Times8, -FRAME_SLOT_SIZE), r14); // -8: stack index
        __ Pushq(r14);
        __ Subq(1, expectedNumArgsReg);
        __ Jne(&lCopyLoop1);
        __ Jmp(&call);
    }

    __ Bind(&checkExpectedArgs); // actualNumArgsReg < 7
    {
        __ Cmp(Immediate(3), expectedNumArgsReg); // 3: expectedNumArgsReg <= 3 jump
        __ Jbe(&call);
        // expectedNumArgsReg > 6, expectedNumArgsReg > actual;NumArgsReg
        __ Subq(3, expectedNumArgsReg); // 3 : skpi func new this
        __ Testb(1, expectedNumArgsReg);
        __ Je(&lCopyExtraUndefineToSp);
        __ Pushq(0); // expectedNumArgsReg is odd need align
        __ Bind(&lCopyExtraUndefineToSp); // copy undefined value to stack
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
        __ Subq(1, expectedNumArgsReg);
        __ Cmp(0, expectedNumArgsReg);
        __ Ja(&lCopyExtraUndefineToSp);
        __ Jmp(&call);
    }
    __ Bind(&call);
    __ Callq(codeAddrReg); // then call jsFunction
}

// * uint64_t JSFastCallWithArgV(uintptr_t glue, uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t prevFp,
//                            size_t callType)
// cc callconv
// * Arguments:
//        %rdi - glue
//        %rsi - func
//        %rdx - this
//        %rcx - actualNumArgs
//        %r8 -  argv

void OptimizedFastCall::JSFastCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSFastCallWithArgV));
    Register sp(rsp);
    Register callsiteSp = __ AvailableRegister2();
    Label align16Bytes;
    Label call;

    __ Movq(sp, callsiteSp);
    __ Addq(Immediate(FRAME_SLOT_SIZE), callsiteSp);   // 8 : 8 means skip pc to get last callsitesp
    OptimizedUnfoldArgVFrameFrameScope scope(assembler); // push frametype and callee save
    __ Movq(rcx, r12);
    __ Movq(r8, rbx);
    Register actualNumArgs(r12);
    Register argV(rbx);

    __ Cmp(0, actualNumArgs);
    __ Jz(&call);
    __ Movq(Operand(argV, 0), rcx); // first arg
    __ Addq(FRAME_SLOT_SIZE, argV);
    __ Addq(-1, actualNumArgs);

    __ Cmp(0, actualNumArgs);
    __ Jz(&call);
    __ Movq(Operand(argV, 0), r8); // second arg
    __ Addq(FRAME_SLOT_SIZE, argV);
    __ Addq(-1, actualNumArgs);

    __ Cmp(0, actualNumArgs);
    __ Jz(&call);
    __ Movq(Operand(argV, 0), r9); // third arg
    __ Addq(FRAME_SLOT_SIZE, argV);
    __ Addq(-1, actualNumArgs);

    __ Cmp(0, actualNumArgs);
    __ Jz(&call);

    __ Testb(1, actualNumArgs);
    __ Je(&align16Bytes);
    __ PushAlignBytes();
    __ Bind(&align16Bytes);
    __ Mov(actualNumArgs, rax);
    CopyArgumentWithArgV(assembler, rax, argV);

    __ Bind(&call);
    Register method = r12;
    Register jsFuncReg = rsi;
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Mov(Operand(jsFuncReg, JSFunctionBase::CODE_ENTRY_OFFSET), rbx); // get codeAddress
    __ Callq(rbx);
}

// cc callconv
// * Arguments:
//        %rdi - glue
//        %rsi - func
//        %rdx - this
//        %rcx - actualNumArgs
//        %r8 -  argv
//        %r9 -  expectedNumArgs

void OptimizedFastCall::JSFastCallWithArgVAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSFastCallWithArgVAndPushArgv));
    Register sp(rsp);
    Register callsiteSp = __ AvailableRegister2();
    Label call;
    Label lCopyExtraAument1;
    Label lCopyExtraUndefineToSp;
    Label lCopyLoop1;
    Label lCopyLoop2;
    Label pushUndefined;
    Label arg1;
    Label arg2;
    Label arg3;
    Label argc;
    Label checkExpectedArgs;

    __ Movq(sp, callsiteSp);
    __ Addq(Immediate(FRAME_SLOT_SIZE), callsiteSp);   // 8 : 8 means skip pc to get last callsitesp
    OptimizedUnfoldArgVFrameFrame1Scope scope(assembler);

    __ Movq(rcx, r12);
    __ Movq(r8, rbx);
    __ Movq(r9, r14);
    Register actualNumArgsReg(r12);
    Register expectedNumArgsReg(r14);
    Register argV(rbx);

    __ Cmp(0, actualNumArgsReg);
    __ Jne(&arg1);
    __ Movq(JSTaggedValue::VALUE_UNDEFINED, rcx);
    __ Movq(JSTaggedValue::VALUE_UNDEFINED, r8);
    __ Movq(JSTaggedValue::VALUE_UNDEFINED, r9);
    __ Jmp(&checkExpectedArgs);

    __ Bind(&arg1);
    {
        __ Movq(Operand(argV, 0), rcx); // first arg
        __ Addq(FRAME_SLOT_SIZE, argV);
        __ Cmp(1, actualNumArgsReg);
        __ Jne(&arg2);
        __ Movq(JSTaggedValue::VALUE_UNDEFINED, r8);
        __ Movq(JSTaggedValue::VALUE_UNDEFINED, r9);
        __ Jmp(&checkExpectedArgs);
    }

    __ Bind(&arg2);
    {
        __ Movq(Operand(argV, 0), r8); // second arg
        __ Addq(FRAME_SLOT_SIZE, argV);
        __ Cmp(2, actualNumArgsReg); // 2: 2 args
        __ Jne(&arg3);
        __ Movq(JSTaggedValue::VALUE_UNDEFINED, r9);
        __ Jmp(&checkExpectedArgs);
    }

    __ Bind(&arg3);
    {
        __ Movq(Operand(argV, 0), r9); // third arg
        __ Addq(FRAME_SLOT_SIZE, argV);
        __ Cmp(3, actualNumArgsReg); // 3: 3 args
        __ Jne(&argc);
        __ Jmp(&checkExpectedArgs);
    }

    __ Bind(&argc); // actualNumArgsReg >=4
    {
        __ Cmpq(expectedNumArgsReg, actualNumArgsReg);
        __ Jb(&pushUndefined);
        __ Subq(3, actualNumArgsReg); // 3: skip above 3 args
        __ Subq(3, expectedNumArgsReg); // 3: skip above 3 args
        __ Testb(1, actualNumArgsReg);
        __ Je(&lCopyLoop2);
        __ Pushq(0);
        __ Bind(&lCopyLoop2);
        __ Movq(Operand(argV, actualNumArgsReg, Scale::Times8, -FRAME_SLOT_SIZE), r13); // -8: stack index
        __ Pushq(r13);
        __ Subq(1, actualNumArgsReg);
        __ Jne(&lCopyLoop2);
        __ Jmp(&call);

        __ Bind(&pushUndefined);
        __ Subq(3, actualNumArgsReg); // 3: skip above 3 args
        __ Subq(3, expectedNumArgsReg); // 3: skip above 3 args
        __ Testb(1, expectedNumArgsReg);
        __ Je(&lCopyExtraAument1);
        __ Pushq(0);
        __ Bind(&lCopyExtraAument1); // copy undefined value to stack
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
        __ Subq(1, expectedNumArgsReg);
        __ Cmpq(actualNumArgsReg, expectedNumArgsReg);
        __ Ja(&lCopyExtraAument1);
        __ Bind(&lCopyLoop1);
        __ Movq(Operand(argV, expectedNumArgsReg, Scale::Times8, -FRAME_SLOT_SIZE), r13); // -8: stack index
        __ Pushq(r13);
        __ Subq(1, expectedNumArgsReg);
        __ Jne(&lCopyLoop1);
        __ Jmp(&call);
    }

    __ Bind(&checkExpectedArgs);
    {
        __ Cmp(Immediate(3), expectedNumArgsReg); // 3:expectedNumArgsReg <= 3 jump
        __ Jbe(&call);
        __ Subq(3, expectedNumArgsReg); // 3 : skpi func new this
        __ Testb(1, expectedNumArgsReg);
        __ Je(&lCopyExtraUndefineToSp);
        __ Pushq(0); // expectedNumArgsReg is odd need align
        __ Bind(&lCopyExtraUndefineToSp); // copy undefined value to stack
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
        __ Subq(1, expectedNumArgsReg);
        __ Cmp(0, expectedNumArgsReg);
        __ Ja(&lCopyExtraUndefineToSp);
        __ Jmp(&call);
    }
    __ Bind(&call);
    Register method = r12;
    Register jsFuncReg = rsi;
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method); // get method
    __ Mov(Operand(jsFuncReg, JSFunctionBase::CODE_ENTRY_OFFSET), rbx); // get codeAddress
    __ Callq(rbx);
}
#undef __
}  // namespace panda::ecmascript::x64