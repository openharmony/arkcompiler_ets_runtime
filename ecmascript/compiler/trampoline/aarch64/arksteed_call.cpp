/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::aarch64 {
#define __ assembler->

// Entry state for ArkSteedCallEntry (CCallConv):
//   x0 = glue
//   x1 = actualNumArgs(user argc only)
//   x2 = argv, laid out as [call-target, new-target, this, arg0, arg1, ...]
//   x3 = prevFp
void ArkSteedCall::ArkSteedCallEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ArkSteedCallEntry));

    Register glueReg = x0;
    Register argvReg = x2;
    Register prevFpReg = x3;
    Register tmpArgV = x7;

    OptimizedCall::PushJSFunctionEntryFrame(assembler, prevFpReg);
    __ UpdateGlueAndReadBarrier(glueReg);

    __ Mov(x20, glueReg);
    __ Mov(tmpArgV, argvReg);
    __ Ldr(x2, MemoryOperand(tmpArgV, 0));
    __ Ldr(x3, MemoryOperand(tmpArgV, FRAME_SLOT_SIZE));
    __ Ldr(x4, MemoryOperand(tmpArgV, DOUBLE_SLOT_SIZE));
    __ Add(tmpArgV, tmpArgV, Immediate(TRIPLE_SLOT_SIZE));
    __ Mov(x5, tmpArgV);

    __ CallAssemblerStub(RTSTUB_ID(SteedCallWithArgVAndPushArgv), false);

    __ Mov(x2, x20);
    OptimizedCall::PopJSFunctionEntryFrame(assembler, x2);
    __ Ret();
}

// Entry state for SteedCallAndPushArgv (generated from AOT_CALL_SIGNATURE):
//   x0 = glue
//   x1 = actualNumArgs(total)
//   x2 = argv
//   x3 = call-target
//   x4 = new-target
//   x5 = this
void ArkSteedCall::SteedCallAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SteedCallAndPushArgv));

    Register jsfunc = x7;
    Register method = x6;
    Register expectedNumArgs = x1;
    Register actualNumArgs = x2;
    Register codeAddr = x3;
    Register argV = x4;
    Register newTarget = x13;
    Register thisObj = x14;

    auto funcSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum();
    __ Ldr(jsfunc, MemoryOperand(sp, funcSlotOffset * FRAME_SLOT_SIZE));
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(codeAddr, MemoryOperand(jsfunc, JSFunction::CODE_ENTRY_OFFSET));
    __ Ldr(expectedNumArgs, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Lsr(expectedNumArgs, expectedNumArgs, Method::NumArgsBits::START_BIT);
    __ And(expectedNumArgs, expectedNumArgs,
        LogicalImmediate::Create(
            Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT, X_REG_SIZE));

    __ Add(argV, sp, Immediate(funcSlotOffset * FRAME_SLOT_SIZE));
    __ Ldr(actualNumArgs, MemoryOperand(sp, 0));
    __ Ldr(newTarget, MemoryOperand(argV, FRAME_SLOT_SIZE));
    __ Ldr(thisObj, MemoryOperand(argV, DOUBLE_SLOT_SIZE));
    __ Add(argV, argV, Immediate(NUM_MANDATORY_JSFUNC_ARGS * FRAME_SLOT_SIZE));

    Register glue = x0;
    Register currentSp = x5;
    Register reservedSlots = x22;
    Register actualArgc = x15;
    Label copyArguments;
    Label invokeSteedCode;

    OptimizedCall::PushOptimizedArgsConfigFrame(assembler);
    __ CalleeSave();
    __ Mov(actualArgc, actualNumArgs);
    __ Sub(actualArgc, actualArgc, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Cmp(expectedNumArgs, actualArgc);
    __ CMov(reservedSlots, expectedNumArgs, actualArgc, Condition::HI);
    // Build the SteedFunctionFrame caller layout expected by GraphBuilder:
    // [argc][call-target][new-target][this][user args...]
    __ Add(reservedSlots, reservedSlots, Immediate(NUM_MANDATORY_JSFUNC_ARGS + 1));
    OptimizedCall::IncreaseStackForArguments(assembler, reservedSlots, currentSp);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register tmp = __ TempRegister1();
        Register undefinedValue = __ TempRegister2();
        __ Subs(tmp, expectedNumArgs, actualArgc);
        __ B(Condition::LS, &copyArguments);
        PushUndefinedWithArgc(assembler, glue, tmp, undefinedValue, currentSp, nullptr, nullptr);
    }
    __ Bind(&copyArguments);
    __ Cbz(actualArgc, &invokeSteedCode);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register argc = __ TempRegister1();
        Register argValue = __ TempRegister2();
        __ Mov(argc, actualArgc);
        PushArgsWithArgv(assembler, glue, argc, argV, argValue, currentSp, &invokeSteedCode, nullptr);
    }
    __ Bind(&invokeSteedCode);
    {
        OptimizedCall::PushMandatoryJSArgs(assembler, jsfunc, thisObj, newTarget, currentSp);
        __ Str(actualNumArgs, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ Mov(x20, jsfunc);
        __ Ldr(x19, MemoryOperand(x20, JSFunction::LEXICAL_ENV_OFFSET));
        __ Blr(codeAddr);
    }

    __ Add(sp, sp, Operand(reservedSlots, UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Mov(x10, sp);
    __ Tst(x10, LogicalImmediate::Create(0xf, X_REG_SIZE));
    Label aligned;
    __ B(Condition::EQ, &aligned);
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ Bind(&aligned);
    __ CalleeRestore();
    OptimizedCall::PopOptimizedArgsConfigFrame(assembler);
    __ Ret();
}

// Entry state for SteedCallWithArgVAndPushArgv (CCallConv variadic stub):
//   x0 = glue
//   x1 = actualNumArgs(user argc only)
//   x2 = call-target
//   x3 = new-target
//   x4 = this
//   x5 = argv for user args only, laid out as [arg0, arg1, ...]
void ArkSteedCall::SteedCallWithArgVAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SteedCallWithArgVAndPushArgv));
    OptimizedCall::GenJSCallWithArgV(assembler, RTSTUB_ID(SteedCallAndPushArgv));
}

#undef __
}  // namespace panda::ecmascript::aarch64
