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

#include "ecmascript/compiler/trampoline/aarch64/common_call.h"

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"
#include "ecmascript/method.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
#define __ assembler->

void CommonCall::PushAsmInterpBridgeFrame(ExtendedAssembler *assembler)
{
    Register fp(X29);
    Register sp(SP);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register frameTypeRegister = __ TempRegister1();

    __ Mov(frameTypeRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME)));
    // 2 : return addr & frame type
    __ Stp(frameTypeRegister, Register(X30), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2 : prevSp & pc
    __ Stp(Register(Zero), Register(FP), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(24));  // 24: skip frame type, prevSp, pc

    if (!assembler->FromInterpreterHandler()) {
        __ CalleeSave();
    }
}

void CommonCall::PopAsmInterpBridgeFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);

    if (!assembler->FromInterpreterHandler()) {
        __ CalleeRestore();
    }
    // 2: prevSp & pc
    __ Ldp(Register(Zero), Register(FP), MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    // 2: return addr & frame type
    __ Ldp(Register(Zero), Register(X30), MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
}


void CommonCall::PushLeaveFrame(ExtendedAssembler *assembler, Register glue)
{
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    Register currentSp(X6);
    Register sp(SP);

    // construct leave frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME)));
    __ PushFpAndLr();
    // 2 : 2 means pairs
    __ Stp(Register(X19), frameType, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(Register(FP), sp, Immediate(DOUBLE_SLOT_SIZE));
    // save to thread currentLeaveFrame_;
    __ Str(Register(FP), MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
}


void CommonCall::PopLeaveFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);
    Register currentSp(X6);
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    // 2 : 2 means pairs
    __ Ldp(Register(X19), frameType, MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ RestoreFpAndLr();
}

void CommonCall::PushArgsWithArgv(ExtendedAssembler *assembler, Register glue, Register argc,
    Register argv, Register op, Register currentSlot, Label *next, Label *stackOverflow)
{
    Label loopBeginning;
    if (next != nullptr) {
        __ Cmp(argc.W(), Immediate(0));
        __ B(Condition::LS, next);
    }
    if (stackOverflow != nullptr) {
        StackOverflowCheck(assembler, glue, currentSlot, argc, op, stackOverflow);
    }
    __ Add(argv, argv, Operand(argc.W(), UXTW, 3));  // 3: argc * 8
    __ Bind(&loopBeginning);
    __ Ldr(op, MemoryOperand(argv, -FRAME_SLOT_SIZE, PREINDEX));  // -8: 8 bytes
    __ Str(op, MemoryOperand(currentSlot, -FRAME_SLOT_SIZE, PREINDEX));  // -8: 8 bytes
    __ Sub(argc.W(), argc.W(), Immediate(1));
    __ Cbnz(argc.W(), &loopBeginning);
}

void CommonCall::PushUndefinedWithArgc(ExtendedAssembler *assembler, Register glue, Register argc, Register temp,
    Register currentSlot, Label *next, Label *stackOverflow)
{
    if (next != nullptr) {
        __ Cmp(argc.W(), Immediate(0));
        __ B(Condition::LE, next);
    }
    if (stackOverflow != nullptr) {
        StackOverflowCheck(assembler, glue, currentSlot, argc, temp, stackOverflow);
    }
    Label loopBeginning;
    __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Bind(&loopBeginning);
    __ Str(temp, MemoryOperand(currentSlot, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Sub(argc.W(), argc.W(), Immediate(1));
    __ Cbnz(argc.W(), &loopBeginning);
}

void CommonCall::StackOverflowCheck(ExtendedAssembler *assembler, Register glue, Register currentSlot,
    Register numArgs, Register op, Label *stackOverflow)
{
    __ Ldr(op, MemoryOperand(glue, JSThread::GlueData::GetStackLimitOffset(false)));
    Label skipThrow;
    __ Sub(op, currentSlot, Operand(op, UXTX, 0));
    __ Cmp(op, Operand(numArgs, LSL, 3));  // 3: each args occupies 8 bytes
    __ B(Condition::GT, &skipThrow);
    __ Ldr(op, MemoryOperand(glue, JSThread::GlueData::GetInterruptVectorOffset(false)));
    __ Tbnz(op, JSThread::CheckSafePointBit::START_BIT, &skipThrow);
    // CheckSafePointBit is false, is actual stack over flow
    __ Ldr(op, MemoryOperand(glue, JSThread::GlueData::GetAllowCrossThreadExecutionOffset(false)));
    __ Cbz(op, stackOverflow);
    __ Bind(&skipThrow);
}
#undef __
}  // panda::ecmascript::aarch64