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

#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value_wrapper.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::aarch64 {
#define __ assembler->

constexpr uint32_t CALL_ARG0 = 0;
constexpr uint32_t CALL_ARG1 = CALL_ARG0 + 1;
constexpr uint32_t CALL_ARG2 = CALL_ARG1 + 1;
constexpr uint32_t DIRECT_USER_ARG_COUNT = 2;

void ArkSteedCall::LoadSteedCallTargetInfo(ExtendedAssembler *assembler, Register jsfunc, Register method,
                                           Register codeAddr, Register expectedNumArgs)
{
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(codeAddr, MemoryOperand(jsfunc, JSFunction::CODE_ENTRY_OFFSET));
    __ Ldr(expectedNumArgs, MemoryOperand(method, Method::CALL_FIELD_OFFSET));
    __ Lsr(expectedNumArgs, expectedNumArgs, Method::NumArgsBits::START_BIT);
    __ And(expectedNumArgs, expectedNumArgs,
        LogicalImmediate::Create(
            Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT, X_REG_SIZE));
}

void ArkSteedCall::CopyUserArgsFromCCallArgs(ExtendedAssembler *assembler, Register actualArgc, Register currentSp,
                                             Label *invokeSteedCode)
{
    Register stackUserArgCount = x11;
    Register stackUserArgEnd = x12;
    Register argValue = x16;
    Register firstArg = x6;
    Register secondArg = x7;
    Label copyStackArgLoop;
    Label copyDirectArgs;
    Label copyDirectArg0;

    __ Cbz(actualArgc, invokeSteedCode);

    // The first two user args are passed in registers; later args are on the entry stack.
    __ Cmp(actualArgc, Immediate(DIRECT_USER_ARG_COUNT));
    __ B(Condition::LS, &copyDirectArgs);

    __ Sub(stackUserArgCount, actualArgc, Immediate(DIRECT_USER_ARG_COUNT));
    __ Add(stackUserArgEnd, fp, Immediate(CommonCall::DOUBLE_SLOT_SIZE));
    __ Add(stackUserArgEnd, stackUserArgEnd, Operand(stackUserArgCount, UXTW, CommonCall::FRAME_SLOT_SIZE_LOG2));
    __ Bind(&copyStackArgLoop);
    __ Ldr(argValue, MemoryOperand(stackUserArgEnd, -CommonCall::FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Str(argValue, MemoryOperand(currentSp, -CommonCall::FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Sub(stackUserArgCount.W(), stackUserArgCount.W(), Immediate(1));
    __ Cbnz(stackUserArgCount.W(), &copyStackArgLoop);

    __ Bind(&copyDirectArgs);
    __ Cmp(actualArgc, Immediate(DIRECT_USER_ARG_COUNT));
    __ B(Condition::LO, &copyDirectArg0);
    __ Str(secondArg, MemoryOperand(currentSp, -CommonCall::FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    __ Bind(&copyDirectArg0);
    __ Str(firstArg, MemoryOperand(currentSp, -CommonCall::FRAME_SLOT_SIZE, AddrMode::PREINDEX));
}

void ArkSteedCall::CopyUserArgsFromArgV(ExtendedAssembler *assembler, Register glue, Register actualArgc,
                                        Register argV, Register currentSp, Label *invokeSteedCode)
{
    __ Cbz(actualArgc, invokeSteedCode);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register argc = __ TempRegister1();
        Register argValue = __ TempRegister2();
        __ Mov(argc, actualArgc);
        CommonCall::PushArgsWithArgv(assembler, glue, argc, argV, argValue, currentSp, invokeSteedCode, nullptr);
    }
}

void ArkSteedCall::PrepareSteedCallFrame(ExtendedAssembler *assembler, Register glue, Register actualNumArgs,
                                         Register expectedNumArgs, Label *copyArguments)
{
    Register currentSp = x5;
    Register reservedSlots = x22;
    Register slotCount = x16;
    Register actualArgc = x15;

    OptimizedCall::PushOptimizedArgsConfigFrame(assembler);
    __ CalleeSave();
    __ Mov(actualArgc, actualNumArgs);
    __ Sub(actualArgc, actualArgc, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Cmp(expectedNumArgs, actualArgc);
    __ CMov(slotCount, expectedNumArgs, actualArgc, Condition::HI);
    // Build the SteedFunctionFrame caller layout expected by GraphBuilder:
    // [argc][call-target][new-target][this][user args...]
    __ Add(slotCount, slotCount, Immediate(NUM_MANDATORY_JSFUNC_ARGS + 1));
    __ Mov(reservedSlots, slotCount);
    OptimizedCall::IncreaseStackForArguments(assembler, slotCount, currentSp);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register tmp = __ TempRegister1();
        Register undefinedValue = __ TempRegister2();
        __ Subs(tmp, expectedNumArgs, actualArgc);
        __ B(Condition::LS, copyArguments);
        CommonCall::PushUndefinedWithArgc(assembler, glue, tmp, undefinedValue, currentSp, nullptr, nullptr);
    }
}

static void FreeSteedCallStack(ExtendedAssembler *assembler)
{
    Register reservedSlots = x22;
    Register slotCount = x16;
    Register actualArgc = x15;

    __ Ldr(actualArgc, MemoryOperand(sp, 0));
    __ Sub(actualArgc, actualArgc, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Ldr(slotCount, MemoryOperand(sp, CommonCall::FRAME_SLOT_SIZE));
    __ Ldr(slotCount, MemoryOperand(slotCount, JSFunction::METHOD_OFFSET));
    __ Ldr(reservedSlots, MemoryOperand(slotCount, Method::CALL_FIELD_OFFSET));
    __ Lsr(reservedSlots, reservedSlots, Method::NumArgsBits::START_BIT);
    __ And(reservedSlots, reservedSlots,
        LogicalImmediate::Create(
            Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT, X_REG_SIZE));
    __ Cmp(reservedSlots, actualArgc);
    __ CMov(reservedSlots, actualArgc, reservedSlots, Condition::LO);
    __ Add(reservedSlots, reservedSlots, Immediate(NUM_MANDATORY_JSFUNC_ARGS + 1));
    __ Add(reservedSlots, reservedSlots, Immediate(1));
    __ And(reservedSlots, reservedSlots, LogicalImmediate::Create(~1ULL, X_REG_SIZE));
    __ Add(sp, sp, Operand(reservedSlots, UXTW, CommonCall::FRAME_SLOT_SIZE_LOG2));
    __ Mov(x10, sp);
    __ Tst(x10, LogicalImmediate::Create(0xf, X_REG_SIZE));
    Label aligned;
    __ B(Condition::EQ, &aligned);
    __ Add(sp, sp, Immediate(CommonCall::FRAME_SLOT_SIZE));
    __ Bind(&aligned);
}

void ArkSteedCall::RestoreSteedCallFrame(ExtendedAssembler *assembler)
{
    __ CalleeRestore();
    OptimizedCall::PopOptimizedArgsConfigFrame(assembler);
    __ Ret();
}

template <typename CopyUserArgs>
void ArkSteedCall::EmitSteedCall(ExtendedAssembler *assembler, Register glue, Register jsfunc, Register codeAddr,
                                 Register newTarget, Register thisObj, Register actualNumArgs,
                                 Register expectedNumArgs, CopyUserArgs copyUserArgs)
{
    Register currentSp = x5;
    Register actualArgc = x15;
    Label copyArguments;
    Label invokeSteedCode;

    PrepareSteedCallFrame(assembler, glue, actualNumArgs, expectedNumArgs, &copyArguments);

    __ Bind(&copyArguments);
    copyUserArgs(actualArgc, currentSp, &invokeSteedCode);

    __ Bind(&invokeSteedCode);
    OptimizedCall::PushMandatoryJSArgs(assembler, jsfunc, thisObj, newTarget, currentSp);
    __ Str(actualNumArgs, MemoryOperand(currentSp, -CommonCall::FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(x20, jsfunc);
    __ Ldr(x19, MemoryOperand(x20, JSFunction::LEXICAL_ENV_OFFSET));
    __ Blr(codeAddr);

    FreeSteedCallStack(assembler);
    RestoreSteedCallFrame(assembler);
}

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
    __ Ldr(x2, MemoryOperand(tmpArgV, CALL_ARG0 * FRAME_SLOT_SIZE));
    __ Ldr(x3, MemoryOperand(tmpArgV, CALL_ARG1 * FRAME_SLOT_SIZE));
    __ Ldr(x4, MemoryOperand(tmpArgV, CALL_ARG2 * FRAME_SLOT_SIZE));
    __ Add(tmpArgV, tmpArgV, Immediate(TRIPLE_SLOT_SIZE));
    __ Mov(x5, tmpArgV);

    __ CallAssemblerStub(RTSTUB_ID(SteedCallWithArgVAndPushArgv), false);

    __ Mov(x2, x20);
    OptimizedCall::PopJSFunctionEntryFrame(assembler, x2);
    __ Ret();
}

// Entry state for ArkSteedEagerDeoptEntry:
//   x17       = glue
//   lr        = function-local continuation
//
// Snapshot layout after reserving 448 bytes:
//   [sp +   0, sp + 128) = x0-x15
//   [sp + 128, sp + 208) = x19-x28; x16-x18 are reserved
//   [sp + 208, sp + 448) = d0-d29
//
// State returned to the function-local continuation:
//   x0        = glue
//   x1        = -1, selects ArkSteed translation materialization in DeoptHandler
//   x2        = zero-valued maybe-accumulator placeholder required by DeoptHandlerAsm
//   x3        = DeoptHandlerAsm address loaded from the eight-byte runtime-stub table
//   sp        = snapshot start
//   lr        = function-local continuation
void ArkSteedCall::ArkSteedEagerDeoptEntry(ExtendedAssembler *assembler)
{
    constexpr int64_t eagerDeoptDispatchMarker = -1;  // ArkSteed dispatch marker
    constexpr int64_t eagerDeoptSnapshotSize = 448;  // 56 eight-byte value slots
    constexpr uint32_t eagerDeoptFirstGeneralRangeEnd = 16;  // x0-x15
    constexpr uint32_t eagerDeoptSecondGeneralRangeBegin = 19;  // x19
    constexpr uint32_t eagerDeoptSecondGeneralRangeEnd = 29;  // One past x28
    constexpr uint32_t eagerDeoptSecondGeneralRangeGap = 3;  // Reserved x16-x18
    constexpr int64_t eagerDeoptFloatingSnapshotOffset = 26 * FRAME_SLOT_SIZE;  // 26 general-register slots
    constexpr uint32_t eagerDeoptFloatingRegisterCount = 30;  // d0-d29
    constexpr int64_t runtimeStubEntrySizeLog2 = 3;  // Eight-byte runtime-stub entries

    __ BindAssemblerStub(RTSTUB_ID(ArkSteedEagerDeoptEntry));
    __ Sub(sp, sp, Operand(Immediate(eagerDeoptSnapshotSize)));

    auto storeGeneralPair = [assembler](uint32_t firstCode, uint32_t firstSlot) {
        Register first = Register::FromCode(static_cast<int8_t>(firstCode));
        Register second = Register::FromCode(static_cast<int8_t>(firstCode + 1U));
        __ Stp(first, second, MemoryOperand(sp, firstSlot * FRAME_SLOT_SIZE));
    };
    for (uint32_t firstCode = 0; firstCode < eagerDeoptFirstGeneralRangeEnd; firstCode += 2U) {
        storeGeneralPair(firstCode, firstCode);
    }
    for (uint32_t firstCode = eagerDeoptSecondGeneralRangeBegin;
         firstCode < eagerDeoptSecondGeneralRangeEnd; firstCode += 2U) {
        storeGeneralPair(firstCode, firstCode - eagerDeoptSecondGeneralRangeGap);
    }

    for (uint32_t firstCode = 0; firstCode < eagerDeoptFloatingRegisterCount; firstCode += 2U) {
        VRegister first = VRegister::FromCode(static_cast<int8_t>(firstCode));
        VRegister second = VRegister::FromCode(static_cast<int8_t>(firstCode + 1U));
        __ Stp(first, second,
               MemoryOperand(sp, eagerDeoptFloatingSnapshotOffset + firstCode * FRAME_SLOT_SIZE));
    }

    __ Mov(x0, x17);
    __ Mov(x1, Immediate(eagerDeoptDispatchMarker));
    __ Mov(x2, Immediate(JSTaggedValue(0).GetRawData()));
    __ Mov(x16, Immediate(RTSTUB_ID(DeoptHandlerAsm)));
    __ Add(x16, x17, Operand(x16, LSL, runtimeStubEntrySizeLog2));
    __ Ldr(x3, MemoryOperand(x16, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Ret();
}

// Entry state for SteedCallAndPushArgv (CCallConv variadic stub):
//   x0 = glue
//   x1 = actualNumArgs(total)
//   x2 = actualArgV / 0
//   x3 = call-target
//   x4 = new-target
//   x5 = this
//   x6 = arg0
//   x7 = arg1
//   [entry sp + 0] = arg2
//   [entry sp + 8] = arg3
//   ...
void ArkSteedCall::SteedCallAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SteedCallAndPushArgv));

    Register glue = x0;
    Register actualNumArgs = x1;
    Register jsfunc = x3;
    Register method = x12;
    Register expectedNumArgs = x11;
    Register codeAddr = x17;
    Register newTarget = x13;
    Register thisObj = x14;

    __ Mov(newTarget, x4);
    __ Mov(thisObj, x5);
    LoadSteedCallTargetInfo(assembler, jsfunc, method, codeAddr, expectedNumArgs);

    EmitSteedCall(assembler, glue, jsfunc, codeAddr, newTarget, thisObj, actualNumArgs, expectedNumArgs,
        [assembler](Register actualArgc, Register currentSp, Label *invokeSteedCode) {
            CopyUserArgsFromCCallArgs(assembler, actualArgc, currentSp, invokeSteedCode);
        });
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

    Register glue = x0;
    Register jsfunc = x2;
    Register method = x6;
    Register expectedNumArgs = x4;
    Register codeAddr = x3;
    Register argV = x12;
    Register newTarget = x13;
    Register thisObj = x14;
    Register actualNumArgs = x1;

    __ Mov(newTarget, x3);
    __ Mov(thisObj, x4);
    __ Mov(argV, x5);
    __ Add(actualNumArgs, actualNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    LoadSteedCallTargetInfo(assembler, jsfunc, method, codeAddr, expectedNumArgs);

    EmitSteedCall(assembler, glue, jsfunc, codeAddr, newTarget, thisObj, actualNumArgs, expectedNumArgs,
        [assembler, glue, argV](Register actualArgc, Register currentSp, Label *invokeSteedCode) {
            CopyUserArgsFromArgV(assembler, glue, actualArgc, argV, currentSp, invokeSteedCode);
        });
}

#undef __
}  // namespace panda::ecmascript::aarch64
