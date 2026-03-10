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

#include "ecmascript/compiler/trampoline/aarch64/common_call.h"

#include "ecmascript/message_string.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
#define __ assembler->

// * uint64_t OptimizedFastCallEntry(uintptr_t glue, uint32_t actualNumArgs, const JSTaggedType argV[],
//                                   uintptr_t prevFp)
// * Arguments:
//        %x0 - glue
//        %x1 - actualNumArgs
//        %x2 - argV
//        %x3 - prevFp
void OptimizedFastCall::OptimizedFastCallEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedFastCallEntry));
    Register glueReg = x0;
    Register argc = x1;
    Register argV = x2;
    Register prevFpReg = x3;

    OptimizedCall::PushJSFunctionEntryFrame (assembler, prevFpReg);
    __ UpdateGlueAndReadBarrier(glueReg);
    __ Mov(x3, argc);
    __ Mov(x4, argV);
    Register tmpArgc = x3;
    Register tmpArgV = x4;

    __ Mov(x20, glueReg);
    __ Ldr(x1, MemoryOperand(tmpArgV, 0));
    __ Ldr(x2, MemoryOperand(tmpArgV, FRAME_SLOT_SIZE));
    __ Add(tmpArgV, tmpArgV, Immediate(DOUBLE_SLOT_SIZE));

    __ CallAssemblerStub(RTSTUB_ID(JSFastCallWithArgV), false);
    __ Mov(x2, x20);
    OptimizedCall::PopJSFunctionEntryFrame(assembler, x2);
    __ Ret();
}

// * uint64_t OptimizedFastCallAndPushArgv(uintptr_t glue, uint32_t expectedNumArgs, uint32_t actualNumArgs,
//                                   uintptr_t codeAddr, uintptr_t argv)
// * Arguments wil CC calling convention:
//         %x0 - glue
//         %x1 - actualNumArgs
//         %x2 - actualArgv
//         %x3 - func
//         %x4  - new target
//         %x5  - this
//         %x6  - arg0
//         %x7  - arg1
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
void OptimizedFastCall::OptimizedFastCallAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedFastCallAndPushArgv));
    Register glue = x0;
    Register actualNumArgs = x1;
    Register actualArgv = x2;
    Register jsfunc = x3;
    Register codeAddr = x4;
    Register currentSp = __ AvailableRegister1();
    Register op = __ AvailableRegister1();
    Label call;
    Label arg4;
    Label arg5;
    Label arg6;
    Label argc;
    Label checkExpectedArgs;
    Label pushUndefined;

    // construct frame
    OptimizedCall::PushOptimizedArgsConfigFrame(assembler);

    __ Mov(__ AvailableRegister3(), x1);
    __ Add(__ AvailableRegister4(), sp, Immediate(4 * FRAME_SLOT_SIZE)); // 4 skip fp lr type x19
    Register actualNumArgsReg = __ AvailableRegister3();
    Register argV = __ AvailableRegister4();

    Register method = __ AvailableRegister1();
    Register expectedNumArgs = __ AvailableRegister2();
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(expectedNumArgs, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Lsr(expectedNumArgs, expectedNumArgs, Method::NumArgsBits::START_BIT);
    __ And(expectedNumArgs, expectedNumArgs,
        LogicalImmediate::Create(
            Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT, X_REG_SIZE));
    __ Add(expectedNumArgs, expectedNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));

    Label arg7;
    Label arg8;
    __ Mov(x1, x3); // func move to argc
    __ Mov(x2, x5); // this move to func
    jsfunc = x1;

    __ Cmp(actualNumArgsReg, Immediate(3)); // 3: 3 args
    __ B(Condition::NE, &arg4);
    __ Mov(x3, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(x4, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(x5, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ B(&checkExpectedArgs);

    __ Bind(&arg4);
    {
        __ Mov(x3, x6);
        __ Cmp(actualNumArgsReg, Immediate(4)); // 4: 4 args
        __ B(Condition::NE, &arg5);
        __ Mov(x4, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x5, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg5);
    {
        __ Mov(x4, x7);
        __ Cmp(actualNumArgsReg, Immediate(5)); // 5: 5 args
        __ B(Condition::NE, &arg6);
        __ Mov(x5, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg6);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(x5, op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(6)); // 6: 6 args
        __ B(Condition::NE, &arg7);
        __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg7);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(x6, op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(7)); // 7: 7 args
        __ B(Condition::NE, &arg8);
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg8);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(x7, op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(8)); // 8: 8 args
        __ B(Condition::NE, &argc);
        __ B(&checkExpectedArgs);
    }

    __ Bind(&argc);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register tmp = __ TempRegister1();
        Register undefinedValue = __ TempRegister2();

        __ Cmp(expectedNumArgs, actualNumArgsReg);
        __ B(Condition::GT, &pushUndefined);
        __ Sub(expectedNumArgs, expectedNumArgs, Immediate(8)); // 8 : register save 8 arg
        __ Sub(actualNumArgsReg, actualNumArgsReg, Immediate(8)); // 8 : register save 8 arg
        OptimizedCall::IncreaseStackForArguments(assembler, actualNumArgsReg, currentSp);
        PushArgsWithArgv(assembler, glue, actualNumArgsReg, argV, undefinedValue, currentSp, nullptr, nullptr);
        __ B(&call);

        __ Bind(&pushUndefined);
        __ Sub(expectedNumArgs, expectedNumArgs, Immediate(8)); // 8 : register save 8 arg
        __ Sub(actualNumArgsReg, actualNumArgsReg, Immediate(8)); // 8 : register save 8 arg
        OptimizedCall::IncreaseStackForArguments(assembler, expectedNumArgs, currentSp);
        __ Sub(tmp, expectedNumArgs, actualNumArgsReg);
        PushUndefinedWithArgc(assembler, glue, tmp, undefinedValue, currentSp, nullptr, nullptr);
        PushArgsWithArgv(assembler, glue, actualNumArgsReg, argV, undefinedValue, currentSp, nullptr, nullptr);
        __ B(&call);
    }

    __ Bind(&checkExpectedArgs);
    {
        __ Cmp(expectedNumArgs, Immediate(8)); // 8 : register save 8 arg
        __ B(Condition::LS, &call);
        __ Sub(expectedNumArgs, expectedNumArgs, Immediate(8)); // 8 : register save 8 arg
        OptimizedCall::IncreaseStackForArguments(assembler, expectedNumArgs, currentSp);
        TempRegister2Scope scope2(assembler);
        Register undefinedValue = __ TempRegister2();
        PushUndefinedWithArgc(assembler, glue, expectedNumArgs, undefinedValue, currentSp, nullptr, nullptr);
        __ B(&call);
    }
    __ Bind(&call);
    TempRegister1Scope scope1(assembler);
    Register method1 = __ TempRegister1();
    __ Ldr(method1, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(x11, MemoryOperand(jsfunc, JSFunction::CODE_ENTRY_OFFSET));
    __ Blr(x11);

    __ Mov(sp, fp);
    __ RestoreFpAndLr();
    __ Ret();
}

// * uint64_t JSFastCallWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType calltarget,
//                                JSTaggedType this, argV)
// * cc calling convention call js function()
// * arguments:
//              %x0 - glue
//              %x1 - call-target
//              %x2 - this
//              %x3 - artual argc
//              %x4 - argv
void OptimizedFastCall::JSFastCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSFastCallWithArgV));
    Register glue = x0;
    Register actualNumArgs = x3;
    Register jsfunc = x1;
    Register thisObj = x2;
    Register currentSp = __ AvailableRegister1();
    Register callsiteSp = __ AvailableRegister2();
    Label call;
    __ Mov(callsiteSp, sp);
    OptimizedCall::PushOptimizedUnfoldArgVFrame(assembler, callsiteSp);
    TempRegister2Scope scope2(assembler);
    Register op = __ TempRegister2();
    Register argC = __ AvailableRegister3();
    Register argV = __ AvailableRegister4();
    __ Mov(argC, actualNumArgs);
    __ Mov(argV, x4);

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(x3, op); // first arg
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(x4, op); // second arg
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(x5, op); // third arg
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(x6, op);
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(x7, op);
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    OptimizedCall::IncreaseStackForArguments(assembler, argC, currentSp);
    PushArgsWithArgv(assembler, glue, argC, argV, op, currentSp, nullptr, nullptr);

    __ Bind(&call);
    TempRegister1Scope scope1(assembler);
    Register method = __ TempRegister1();
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(x11, MemoryOperand(jsfunc, JSFunctionBase::CODE_ENTRY_OFFSET));
    __ Blr(x11);

    __ Mov(sp, fp);
    __ RestoreFpAndLr();
    __ Ret();
}

// * Arguments:
//        %x0 - glue
//        %x1 - func
//        %x2 - this
//        %x3 - actualNumArgs
//        %x4 -  argv
//        %x5 -  expectedNumArgs
void OptimizedFastCall::JSFastCallWithArgVAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSFastCallWithArgVAndPushArgv));
    Register glue = x0;
    Register jsfunc = x1;
    Register thisObj = x2;
    Register currentSp = __ AvailableRegister1();
    Register op = __ AvailableRegister1();
    Register callsiteSp = __ AvailableRegister2();
    Label call;
    Label arg1;
    Label arg2;
    Label arg3;
    Label arg4;
    Label arg5;
    Label argc;
    Label checkExpectedArgs;
    Label pushUndefined;
    __ Mov(callsiteSp, sp);
    OptimizedCall::PushOptimizedUnfoldArgVFrame(assembler, callsiteSp);
    Register actualNumArgsReg = __ AvailableRegister3();
    Register argV = __ AvailableRegister4();
    Register expectedNumArgs = __ AvailableRegister2();
    __ Mov(actualNumArgsReg, x3);
    __ Mov(argV, x4);
    __ Mov(expectedNumArgs, x5);

    __ Cmp(actualNumArgsReg, Immediate(0));
    __ B(Condition::NE, &arg1);
    __ Mov(x3, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(x4, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(x5, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ B(&checkExpectedArgs);

    __ Bind(&arg1);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(x3, op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(1));
        __ B(Condition::NE, &arg2);
        __ Mov(x4, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x5, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg2);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(x4, op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(2)); // 2: 2 args
        __ B(Condition::NE, &arg3);
        __ Mov(x5, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg3);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(x5, op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(3)); // 3: 3 args
        __ B(Condition::NE, &arg4);
        __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg4);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(x6, op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(4)); // 4: 4 args
        __ B(Condition::NE, &arg5);
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg5);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(x7, op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(5)); // 5: 5 args
        __ B(Condition::NE, &argc);
        __ B(&checkExpectedArgs);
    }

    __ Bind(&argc);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register tmp = __ TempRegister1();
        Register undefinedValue = __ TempRegister2();

        __ Cmp(expectedNumArgs, actualNumArgsReg);
        __ B(Condition::GT, &pushUndefined);
        __ Sub(expectedNumArgs, expectedNumArgs, Immediate(5)); // 5 : register save 5 arg
        __ Sub(actualNumArgsReg, actualNumArgsReg, Immediate(5)); // 5 : register save 5 arg
        OptimizedCall::IncreaseStackForArguments(assembler, actualNumArgsReg, currentSp);
        PushArgsWithArgv(assembler, glue, actualNumArgsReg, argV, undefinedValue, currentSp, nullptr, nullptr);
        __ B(&call);

        __ Bind(&pushUndefined);
        __ Sub(expectedNumArgs, expectedNumArgs, Immediate(5)); // 5 : register save 5 arg
        __ Sub(actualNumArgsReg, actualNumArgsReg, Immediate(5)); // 5 : register save 5 arg
        OptimizedCall::IncreaseStackForArguments(assembler, expectedNumArgs, currentSp);
        __ Sub(tmp, expectedNumArgs, actualNumArgsReg);
        PushUndefinedWithArgc(assembler, glue, tmp, undefinedValue, currentSp, nullptr, nullptr);
        PushArgsWithArgv(assembler, glue, actualNumArgsReg, argV, undefinedValue, currentSp, nullptr, nullptr);
        __ B(&call);
    }

    __ Bind(&checkExpectedArgs);
    {
        __ Cmp(expectedNumArgs, Immediate(5)); // 5 : register save 5 arg
        __ B(Condition::LS, &call);
        __ Sub(expectedNumArgs, expectedNumArgs, Immediate(5)); // 5 : register save 5 arg
        OptimizedCall::IncreaseStackForArguments(assembler, expectedNumArgs, currentSp);
        TempRegister2Scope scope2(assembler);
        Register undefinedValue = __ TempRegister2();
        PushUndefinedWithArgc(assembler, glue, expectedNumArgs, undefinedValue, currentSp, nullptr, nullptr);
        __ B(&call);
    }

    __ Bind(&call);
    TempRegister1Scope scope1(assembler);
    Register method = __ TempRegister1();
    __ Ldr(method, MemoryOperand(x1, JSFunction::METHOD_OFFSET));
    __ Ldr(x11, MemoryOperand(x1, JSFunction::CODE_ENTRY_OFFSET));
    __ Blr(x11);

    __ Mov(sp, fp);
    __ RestoreFpAndLr();
    __ Ret();
}
#undef __
}  // panda::ecmascript::aarch64