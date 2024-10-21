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

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/argument_accessor.h"
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
    Register glueReg(X0);
    Register argc(X1);
    Register argV(X2);
    Register prevFpReg(X3);
    Register sp(SP);

    OptimizedCall::PushJSFunctionEntryFrame (assembler, prevFpReg);
    __ Mov(Register(X3), argc);
    __ Mov(Register(X4), argV);
    Register tmpArgc(X3);
    Register tmpArgV(X4);

    __ Mov(Register(X20), glueReg);
    __ Ldr(Register(X1), MemoryOperand(tmpArgV, 0));
    __ Ldr(Register(X2), MemoryOperand(tmpArgV, FRAME_SLOT_SIZE));
    __ Add(tmpArgV, tmpArgV, Immediate(DOUBLE_SLOT_SIZE));

    __ CallAssemblerStub(RTSTUB_ID(JSFastCallWithArgV), false);
    __ Mov(Register(X2), Register(X20));
    OptimizedCall::PopJSFunctionEntryFrame(assembler, Register(X2));
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
    Register glue(X0);
    Register actualNumArgs(X1);
    Register actualArgv(X2);
    Register jsfunc(X3);
    Register codeAddr(X4);
    Register sp(SP);
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

    __ Mov(__ AvailableRegister3(), Register(X1));
    __ Add(__ AvailableRegister4(), sp, Immediate(4 * FRAME_SLOT_SIZE)); // 4 skip fp lr type x19
    Register actualNumArgsReg = __ AvailableRegister3();
    Register argV = __ AvailableRegister4();

    Register method = __ AvailableRegister1();
    Register expectedNumArgs = __ AvailableRegister2();
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(expectedNumArgs, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Lsr(expectedNumArgs, expectedNumArgs, MethodLiteral::NumArgsBits::START_BIT);
    __ And(expectedNumArgs, expectedNumArgs,
        LogicalImmediate::Create(
            MethodLiteral::NumArgsBits::Mask() >> MethodLiteral::NumArgsBits::START_BIT, RegXSize));
    __ Add(expectedNumArgs, expectedNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));

    Label arg7;
    Label arg8;
    __ Mov(Register(X1), Register(X3)); // func move to argc
    __ Mov(Register(X2), Register(X5)); // this move to func
    jsfunc = Register(X1);

    __ Cmp(actualNumArgsReg, Immediate(3)); // 3: 3 args
    __ B(Condition::NE, &arg4);
    __ Mov(Register(X3), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(Register(X4), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(Register(X5), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ B(&checkExpectedArgs);

    __ Bind(&arg4);
    {
        __ Mov(Register(X3), Register(X6));
        __ Cmp(actualNumArgsReg, Immediate(4)); // 4: 4 args
        __ B(Condition::NE, &arg5);
        __ Mov(Register(X4), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X5), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg5);
    {
        __ Mov(Register(X4), Register(X7));
        __ Cmp(actualNumArgsReg, Immediate(5)); // 5: 5 args
        __ B(Condition::NE, &arg6);
        __ Mov(Register(X5), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg6);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(Register(X5), op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(6)); // 6: 6 args
        __ B(Condition::NE, &arg7);
        __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg7);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(Register(X6), op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(7)); // 7: 7 args
        __ B(Condition::NE, &arg8);
        __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg8);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(Register(X7), op);
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
    __ Ldr(X11, MemoryOperand(jsfunc, JSFunction::CODE_ENTRY_OFFSET));
    __ Blr(X11);

    __ Mov(Register(SP), Register(FP));
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
    Register sp(SP);
    Register glue(X0);
    Register actualNumArgs(X3);
    Register jsfunc(X1);
    Register thisObj(X2);
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
    __ Mov(argV, Register(X4));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(Register(X3), op); // first arg
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(Register(X4), op); // second arg
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(Register(X5), op); // third arg
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(Register(X6), op);
    __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
    __ Sub(argC, argC, Immediate(1));

    __ Cmp(argC, Immediate(0));
    __ B(Condition::EQ, &call);
    __ Ldr(op, MemoryOperand(argV, 0));
    __ Mov(Register(X7), op);
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
    __ Ldr(X11, MemoryOperand(jsfunc, JSFunctionBase::CODE_ENTRY_OFFSET));
    __ Blr(X11);

    __ Mov(Register(SP), Register(FP));
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
    Register sp(SP);
    Register glue(X0);
    Register jsfunc(X1);
    Register thisObj(X2);
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
    __ Mov(actualNumArgsReg, Register(X3));
    __ Mov(argV, Register(X4));
    __ Mov(expectedNumArgs, Register(X5));

    __ Cmp(actualNumArgsReg, Immediate(0));
    __ B(Condition::NE, &arg1);
    __ Mov(Register(X3), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(Register(X4), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(Register(X5), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ B(&checkExpectedArgs);

    __ Bind(&arg1);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(Register(X3), op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(1));
        __ B(Condition::NE, &arg2);
        __ Mov(Register(X4), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X5), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg2);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(Register(X4), op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(2)); // 2: 2 args
        __ B(Condition::NE, &arg3);
        __ Mov(Register(X5), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg3);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(Register(X5), op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(3)); // 3: 3 args
        __ B(Condition::NE, &arg4);
        __ Mov(Register(X6), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg4);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(Register(X6), op);
        __ Add(argV, argV, Immediate(FRAME_SLOT_SIZE));
        __ Cmp(actualNumArgsReg, Immediate(4)); // 4: 4 args
        __ B(Condition::NE, &arg5);
        __ Mov(Register(X7), Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(&checkExpectedArgs);
    }

    __ Bind(&arg5);
    {
        __ Ldr(op, MemoryOperand(argV, 0));
        __ Mov(Register(X7), op);
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
    __ Ldr(method, MemoryOperand(X1, JSFunction::METHOD_OFFSET));
    __ Ldr(X11, MemoryOperand(X1, JSFunction::CODE_ENTRY_OFFSET));
    __ Blr(X11);

    __ Mov(Register(SP), Register(FP));
    __ RestoreFpAndLr();
    __ Ret();
}
#undef __
}  // panda::ecmascript::aarch64