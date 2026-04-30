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

#include "ecmascript/compiler/trampoline/x64/common_call.h"

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::x64 {
#define __ assembler->

// Entry state for ArkSteedCallEntry (CCallConv):
//   rdi = glue
//   rsi = actualNumArgs(user argc only)
//   rdx = argv, laid out as [call-target, new-target, this, arg0, arg1, ...]
//   rcx = prevFp
//
//   Stack on entry (high address -> low address):
//               +--------------------------+
//   rsp ------->|      return address      |
//               +--------------------------+
//
// This stub first pushes the optimized entry frame, then rewrites the incoming C++ arguments into the Steed argv-call
// convention expected by SteedCallWithArgVAndPushArgv:
//   rdi = glue
//   rsi = actualNumArgs(user argc only)
//   rdx = call-target
//   rcx = new-target
//   r8  = this
//   r9  = argv for user args only, laid out as [arg0, arg1, ...]
void ArkSteedCall::ArkSteedCallEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ArkSteedCallEntry));

    Register glueReg = rdi;
    Register actualNumArgsReg = rsi;
    Register argvReg = rdx;
    Register prevFpReg = rcx;

    // Push OPTIMIZED_ENTRY_FRAME
    OptimizedCall::PushJSFunctionEntryFrame(assembler, prevFpReg);
    __ UpdateReadBarrier(glueReg);

    // Load func, newTarget, this from argv
    Register func = rax;
    Register newTarget = r10;
    Register thisObj = r11;
    __ Movq(Operand(argvReg, 0), func);
    __ Movq(Operand(argvReg, FRAME_SLOT_SIZE), newTarget);
    __ Movq(Operand(argvReg, 2 * FRAME_SLOT_SIZE), thisObj);  // 2: thisObj argv slot index

    // Compute user argv pointer (skip func, newTarget, this)
    Register userArgv = r9;
    __ Movq(argvReg, userArgv);
    __ Addq(NUM_MANDATORY_JSFUNC_ARGS * FRAME_SLOT_SIZE, userArgv);

    // Set up CCallConv for SteedCallWithArgVAndPushArgv:
    //   rdi=glue, rsi=actualNumArgs(total), rdx=func, rcx=newTarget, r8=this, r9=argv
    __ Movq(func, rdx);
    __ Movq(newTarget, rcx);
    __ Movq(thisObj, r8);
    // rdi=glue, rsi=actualNumArgsReg, r9=userArgv already set

    __ CallAssemblerStub(RTSTUB_ID(SteedCallWithArgVAndPushArgv), false);

    // Pop OPTIMIZED_ENTRY_FRAME
    __ Popq(prevFpReg);
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Popq(rbp);
    __ Popq(glueReg);
    __ PopCppCalleeSaveRegisters();
    __ Movq(prevFpReg, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Ret();
}

// Entry state for SteedCallAndPushArgv (generated from AOT_CALL_SIGNATURE):
//   rdi = glue
//   rsi = actualNumArgs(total)
//   rdx = argv (unused for the fixed-arg path, passed as 0)
//   rcx = call-target
//   r8  = new-target
//   r9  = this
//
//   Stack on entry (high address -> low address):
//               +--------------------------+
//               |        arg[N-1]          |
//               +--------------------------+
//               |        ...               |
//               +--------------------------+
//               |        arg1              |
//               +--------------------------+
//               |        arg0              |
//               +--------------------------+
//               |        this              |
//               +--------------------------+
//               |      new-target          |
//               +--------------------------+
//               |      call-target         |
//               +--------------------------+
//               |        argv              |
//               +--------------------------+
//               |      actualNumArgs       |
//               +--------------------------+
//   rsp ------->|      return address      |
//               +--------------------------+
//
// This stub materializes the Steed caller-side argument area and then calls the Steed machine-code entry directly.
// Before the call, the caller-visible layout is normalized to:
//   [argc][call-target][new-target][this][arg0][arg1]...
// and r12/rbx/r13 are prepared for the Steed prologue as:
//   r12 = jsFunc, rbx = lexicalEnv, r13 = actual user argc
void ArkSteedCall::SteedCallAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SteedCallAndPushArgv));

    Register jsFuncReg = rdi;
    Register method = r9;
    Register codeAddrReg = rsi;

    auto funcSlotOffset = kungfu::ArgumentAccessor::GetExtraArgsNum() + 1;
    __ Movq(Operand(rsp, funcSlotOffset * FRAME_SLOT_SIZE), jsFuncReg);
    __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), method);
    __ Mov(Operand(jsFuncReg, JSFunctionBase::CODE_ENTRY_OFFSET), codeAddrReg);

    Register methodCallField = rcx;
    __ Mov(Operand(method, Method::CALL_FIELD_OFFSET), methodCallField);
    __ Shr(Method::NumArgsBits::START_BIT, methodCallField);
    __ Andl(((1LU << Method::NumArgsBits::SIZE) - 1), methodCallField);
    __ Addl(NUM_MANDATORY_JSFUNC_ARGS, methodCallField);

    Register actualNumArgsReg = rdx;
    __ Movl(Operand(rsp, FRAME_SLOT_SIZE), actualNumArgsReg);
    __ Movq(rsp, r8);
    Register argvReg = r8;
    __ Addq(funcSlotOffset * FRAME_SLOT_SIZE, argvReg);

    Register newTarget = r10;
    Register thisObj = r11;
    __ Movq(Operand(argvReg, FRAME_SLOT_SIZE), newTarget);
    __ Movq(Operand(argvReg, 2 * FRAME_SLOT_SIZE), thisObj);  // 2: thisObj argv slot index
    __ Addq(NUM_MANDATORY_JSFUNC_ARGS * FRAME_SLOT_SIZE, argvReg);

    Register expectedNumArgsReg = rcx;
    Register actualArgcReg = rax;

    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);
    __ Pushq(r12);
    __ Pushq(r13);
    __ Pushq(r14);
    __ Pushq(rbx);
    // Reserve one temp spill slot to keep the args-config-frame layout aligned with the optimized path.
    __ Pushq(rax);

    __ Movl(actualNumArgsReg, actualArgcReg);
    __ Subq(NUM_MANDATORY_JSFUNC_ARGS, actualArgcReg);
    __ Subq(NUM_MANDATORY_JSFUNC_ARGS, expectedNumArgsReg);

    Label lCopyExtraArg1;
    Label lCopyLoop1;
    Label lCopyLoop2;
    Label lPopFrame1;
    Label pushUndefined;
    Label commonCall;

    __ Cmpq(expectedNumArgsReg, actualArgcReg);
    __ Jb(&pushUndefined);

    __ Movl(actualArgcReg, r14);
    __ Cmpq(0, actualArgcReg);
    __ Je(&commonCall);

    __ Testb(1, r14);
    __ Je(&lCopyLoop2);
    __ Pushq(0);

    __ Bind(&lCopyLoop2);
    __ Movq(Operand(argvReg, r14, Scale::Times8, -FRAME_SLOT_SIZE), rbx);
    __ Pushq(rbx);
    __ Addq(-1, r14);
    __ Jne(&lCopyLoop2);
    __ Movl(actualArgcReg, r14);
    __ Jmp(&commonCall);

    __ Bind(&pushUndefined);
    __ Movl(expectedNumArgsReg, r14);
    __ Testb(1, r14);
    __ Je(&lCopyExtraArg1);
    __ Pushq(0);

    __ Bind(&lCopyExtraArg1);
    __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
    __ Addq(-1, expectedNumArgsReg);
    __ Cmpq(actualArgcReg, expectedNumArgsReg);
    __ Ja(&lCopyExtraArg1);

    // No user arguments were passed. After padding with `undefined`, there is
    // no argv payload left to copy, so avoid reading argv[-1].
    __ Cmpq(0, actualArgcReg);
    __ Je(&commonCall);

    __ Bind(&lCopyLoop1);
    __ Movq(Operand(argvReg, expectedNumArgsReg, Scale::Times8, -FRAME_SLOT_SIZE), rbx);
    __ Pushq(rbx);
    __ Addq(-1, expectedNumArgsReg);
    __ Jne(&lCopyLoop1);
    __ Jmp(&commonCall);

    __ Bind(&commonCall);
    // SteedFunctionFrame caller layout: [argc][call-target][newTarget][this][user args...]
    __ Pushq(thisObj);
    __ Pushq(newTarget);
    __ Pushq(jsFuncReg);
    __ Pushq(actualNumArgsReg);

    // Steed prologue consumes jsfunc/context/user argc from fixed registers.
    __ Movq(jsFuncReg, r12);
    __ Movq(Operand(r12, JSFunction::LEXICAL_ENV_OFFSET), rbx);
    __ Movl(actualArgcReg, r13);

    __ Callq(codeAddrReg);

    __ Addq(4 * FRAME_SLOT_SIZE, rsp);  // 4: slots for pushed registers
    __ Leaq(Operand(r14, Scale::Times8, 0), rbx);
    __ Addq(rbx, rsp);
    __ Testb(1, r14);
    __ Je(&lPopFrame1);
    __ Addq(FRAME_SLOT_SIZE, rsp);

    __ Bind(&lPopFrame1);
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Popq(rbx);
    __ Popq(r14);
    __ Popq(r13);
    __ Popq(r12);
    __ Addq(FRAME_SLOT_SIZE, rsp);
    __ Pop(rbp);
    __ Ret();
}

// Entry state for SteedCallWithArgVAndPushArgv (CCallConv variadic stub):
//   rdi = glue
//   rsi = actualNumArgs(user argc only)
//   rdx = call-target
//   rcx = new-target
//   r8  = this
//   r9  = argv for user args only, laid out as [arg0, arg1, ...]
//
//   Stack on entry (high address -> low address):
//               +--------------------------+
//   rsp ------->|      return address      |
//               +--------------------------+
//
// This stub unfolds the user argv onto the stack with the same calling convention used by JSCallWithArgVAndPushArgv,
// then tail-calls SteedCallAndPushArgv. After GenJSCallWithArgV finishes, the callee sees the same entry stack/register
// state documented above for SteedCallAndPushArgv.
void ArkSteedCall::SteedCallWithArgVAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SteedCallWithArgVAndPushArgv));
    OptimizedCall::GenJSCallWithArgV(assembler, RTSTUB_ID(SteedCallAndPushArgv));
}

#undef __
}  // namespace panda::ecmascript::x64
