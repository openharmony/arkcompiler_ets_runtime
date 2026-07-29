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

#include "ecmascript/arksteed/arksteed_deopt_abi.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value_wrapper.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::x64 {
#define __ assembler->

namespace eager_deopt_abi = arksteed::x64_eager_deopt_abi;

constexpr int64_t CALL_ARG0 = 0;
constexpr int64_t CALL_ARG1 = CALL_ARG0 + 1;
constexpr int64_t CALL_ARG2 = CALL_ARG1 + 1;

constexpr int64_t CPP_CALLEE_SAVE_REGISTER_SLOTS = 5;
constexpr int64_t NEW_TARGET_LOCAL_SLOT_OFFSET_FROM_FP =
    -(CPP_CALLEE_SAVE_REGISTER_SLOTS + CALL_ARG2) * CommonCall::FRAME_SLOT_SIZE;

void ArkSteedCall::LoadSteedCallTargetInfo(ExtendedAssembler *assembler, Register jsfunc, Register method,
                                           Register codeAddr, Register expectedNumArgs)
{
    __ Movq(Operand(jsfunc, JSFunction::METHOD_OFFSET), method);
    __ Movq(Operand(jsfunc, JSFunction::CODE_ENTRY_OFFSET), codeAddr);
    __ Movq(Operand(method, Method::CALL_FIELD_OFFSET), expectedNumArgs);
    __ Shr(Immediate(Method::NumArgsBits::START_BIT), expectedNumArgs);
    __ Andl(static_cast<int32_t>((1LU << Method::NumArgsBits::SIZE) - 1), expectedNumArgs);
}

void ArkSteedCall::CopyUserArgsFromCCallArgs(ExtendedAssembler *assembler, Register actualArgc, Register currentSp,
                                             Label *invokeSteedCode)
{
    // On x64, all 6 C ABI argument registers are consumed by stub parameters
    // (rdi=glue, rsi=actualNumArgs, rdx=actualArgV, rcx=jsfunc, r8=newTarget, r9=this),
    // so user args (arg0, arg1, ...) are all on the entry stack at [rbp + 16].
    Register stackArgV = rdx;
    Register argValue = rax;

    __ Cmpq(0, actualArgc);
    __ Je(invokeSteedCode);

    // stackArgV = rbp + 16 + actualArgc * 8, points to one past the last arg
    __ Leaq(Operand(rbp, actualArgc, Scale::Times8, CommonCall::DOUBLE_SLOT_SIZE), stackArgV);

    Label copyLoop;
    __ Bind(&copyLoop);
    __ Subq(CommonCall::FRAME_SLOT_SIZE, stackArgV);
    __ Movq(Operand(stackArgV, 0), argValue);
    __ Subq(CommonCall::FRAME_SLOT_SIZE, currentSp);
    __ Movq(argValue, Operand(currentSp, 0));
    __ Subq(1, actualArgc);
    __ Jne(&copyLoop);
}

void ArkSteedCall::CopyUserArgsFromArgV(ExtendedAssembler *assembler, Register glue, Register actualArgc,
                                        Register argV, Register currentSp, Label *invokeSteedCode)
{
    __ Cmpq(0, actualArgc);
    __ Je(invokeSteedCode);
    {
        Register argValue = rax;

        Label copyLoop;
        __ Bind(&copyLoop);
        __ Movq(Operand(argV, actualArgc, Scale::Times8, -CommonCall::FRAME_SLOT_SIZE), argValue);
        __ Subq(CommonCall::FRAME_SLOT_SIZE, currentSp);
        __ Movq(argValue, Operand(currentSp, 0));
        __ Subq(1, actualArgc);
        __ Jne(&copyLoop);
    }
}

static void PrepareSteedCallFrame(ExtendedAssembler *assembler, Register glue, Register jsfunc, Register codeAddr,
                                  Register newTarget, Register thisObj, Register actualNumArgs,
                                  Register expectedNumArgs, Label *copyArguments)
{
    __ Pushq(rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME));
    __ Leaq(Operand(rsp, CommonCall::FRAME_SLOT_SIZE), rbp);
    __ PushCppCalleeSaveRegisters();  // saves r12, r13, r14, r15, rbx

    // Keep newTarget in a local bridge slot. r13 is reserved for user argc in
    // the bridge path, so it cannot be used to preserve newTarget.
    __ Subq(CommonCall::FRAME_SLOT_SIZE, rsp);
    __ Movq(newTarget, Operand(rsp, 0));

    // Move persistent values into callee-saved registers.
    __ Movq(jsfunc, r12);
    __ Movq(thisObj, r14);
    __ Movq(codeAddr, r15);

    Register currentSp = r10;
    Register reservedSlots = rbx;
    Register actualArgc = r8;

    __ Movq(actualNumArgs, actualArgc);
    __ Subq(NUM_MANDATORY_JSFUNC_ARGS, actualArgc);
    __ Movq(actualArgc, r13);

    // reservedSlots = max(expectedNumArgs, actualArgc)
    Label slotCountDone;
    __ Movq(expectedNumArgs, reservedSlots);
    __ Cmpq(reservedSlots, actualArgc);  // actualArgc - reservedSlots
    __ Jle(&slotCountDone);              // actualArgc <= reservedSlots → keep expected
    __ Movq(actualArgc, reservedSlots);  // actualArgc > reservedSlots → use actual
    __ Bind(&slotCountDone);
    // reservedSlots += NUM_MANDATORY_JSFUNC_ARGS + 1. Keep the logical size in
    // rax, then make the reserved slot count odd so rsp is 16-byte aligned
    // before the generated Steed code call.
    __ Addq(NUM_MANDATORY_JSFUNC_ARGS + 1, reservedSlots);

    // 4. Reserve output stack area and set up currentSp. If a padding slot was
    // added, currentSp skips it so the final argv layout remains
    // [argc][jsfunc][newTarget][this][user args...].
    __ Leaq(Operand(reservedSlots, Scale::Times8, 0), rax);
    __ Or(x64::Immediate(1), reservedSlots);
    __ Leaq(Operand(reservedSlots, Scale::Times8, 0), currentSp);
    __ Subq(currentSp, rsp);
    __ Movq(rsp, currentSp);
    __ Addq(rax, currentSp);

    // 5. Push undefined for missing args: when expectedNumArgs > actualArgc.
    __ Movq(expectedNumArgs, rax);
    __ Subq(actualArgc, rax);  // rax = expectedNumArgs - actualArgc
    __ Cmpq(0, rax);
    __ Jle(copyArguments);
    {
        Label fillUndefined;
        Register undefinedValue = rdx;
        __ Movabs(JSTaggedValue::VALUE_UNDEFINED, undefinedValue);
        __ Bind(&fillUndefined);
        __ Subq(CommonCall::FRAME_SLOT_SIZE, currentSp);
        __ Movq(undefinedValue, Operand(currentSp, 0));
        __ Subq(1, rax);
        __ Jne(&fillUndefined);
    }
}

static void InvokeSteedCode(ExtendedAssembler *assembler, Register currentSp, Register actualNumArgs,
                            Register reservedSlots)
{
    // Push order must match PushMandatoryJSArgs:
    // final SteedFunctionFrame caller layout is [argc][jsfunc][newTarget][this].
    __ Subq(CommonCall::FRAME_SLOT_SIZE, currentSp);
    __ Movq(r14, Operand(currentSp, 0));  // thisObj
    __ Subq(CommonCall::FRAME_SLOT_SIZE, currentSp);
    __ Movq(Operand(rbp, NEW_TARGET_LOCAL_SLOT_OFFSET_FROM_FP), rax);
    __ Movq(rax, Operand(currentSp, 0));  // newTarget
    // The generated callee may trigger a moving GC. Do not read the call-target
    // object after the call just to recompute how many slots to release. The
    // newTarget local is dead after it has been copied to the outgoing argv.
    __ Movq(reservedSlots, Operand(rbp, NEW_TARGET_LOCAL_SLOT_OFFSET_FROM_FP));
    __ Subq(CommonCall::FRAME_SLOT_SIZE, currentSp);
    __ Movq(r12, Operand(currentSp, 0));  // jsfunc
    __ Subq(CommonCall::FRAME_SLOT_SIZE, currentSp);
    __ Movq(actualNumArgs, Operand(currentSp, 0));  // total argc

    __ Movq(currentSp, rsp);
    __ Movq(Operand(r12, JSFunction::LEXICAL_ENV_OFFSET), rbx);
    __ Callq(r15);  // call codeAddr
}

static void FreeSteedCallStack(ExtendedAssembler *assembler, Register actualArgc, Register reservedSlots)
{
    (void)actualArgc;
    __ Movq(Operand(rbp, NEW_TARGET_LOCAL_SLOT_OFFSET_FROM_FP), reservedSlots);
    __ Leaq(Operand(reservedSlots, Scale::Times8, 0), rdx);
    __ Addq(rdx, rsp);
}

static void RestoreSteedCallFrame(ExtendedAssembler *assembler)
{
    __ Addq(CommonCall::FRAME_SLOT_SIZE, rsp);   // skip local newTarget slot
    __ PopCppCalleeSaveRegisters();
    __ Addq(CommonCall::FRAME_SLOT_SIZE, rsp);   // skip frameType
    __ Popq(rbp);
    __ Ret();
}

template <typename CopyUserArgs>
void ArkSteedCall::EmitSteedCall(ExtendedAssembler *assembler, Register glue, Register jsfunc, Register codeAddr,
                                 Register newTarget, Register thisObj, Register actualNumArgs,
                                 Register expectedNumArgs, CopyUserArgs copyUserArgs)
{
    // Persistent assignment after PushCppCalleeSaveRegisters:
    // r12=jsfunc, r13=actual user argc, r14=thisObj, r15=codeAddr, rbx=reservedSlots.
    Register currentSp = r10;
    Register reservedSlots = rbx;
    Register actualArgc = r8;
    Label copyArguments;
    Label invokeSteedCode;

    PrepareSteedCallFrame(assembler, glue, jsfunc, codeAddr, newTarget, thisObj, actualNumArgs, expectedNumArgs,
                          &copyArguments);

    __ Bind(&copyArguments);
    copyUserArgs(actualArgc, currentSp, &invokeSteedCode);

    __ Bind(&invokeSteedCode);
    InvokeSteedCode(assembler, currentSp, actualNumArgs, reservedSlots);

    FreeSteedCallStack(assembler, actualArgc, reservedSlots);
    RestoreSteedCallFrame(assembler);
}

// Entry state for ArkSteedCallEntry (CCallConv):
//   rdi = glue
//   rsi = actualNumArgs(user argc only)
//   rdx = argv, laid out as [call-target, new-target, this, arg0, arg1, ...]
//   rcx = prevFp
//
// This stub pushes the optimized entry frame, then rewrites the incoming C++ arguments
// into the Steed argv-call convention expected by SteedCallWithArgVAndPushArgv:
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
    __ Movq(Operand(argvReg, CALL_ARG0 * FRAME_SLOT_SIZE), func);
    __ Movq(Operand(argvReg, CALL_ARG1 * FRAME_SLOT_SIZE), newTarget);
    __ Movq(Operand(argvReg, CALL_ARG2 * FRAME_SLOT_SIZE), thisObj);

    // Compute user argv pointer (skip func, newTarget, this)
    Register userArgv = r9;
    __ Movq(argvReg, userArgv);
    __ Addq(NUM_MANDATORY_JSFUNC_ARGS * FRAME_SLOT_SIZE, userArgv);

    // Preserve glue in callee-saved r12 across the call.
    __ Movq(glueReg, r12);

    // Set up CCallConv for SteedCallWithArgVAndPushArgv:
    //   rdi=glue, rsi=actualNumArgs(user), rdx=func, rcx=newTarget, r8=this, r9=argv
    __ Movq(func, rdx);
    __ Movq(newTarget, rcx);
    __ Movq(thisObj, r8);

    __ CallAssemblerStub(RTSTUB_ID(SteedCallWithArgVAndPushArgv), false);

    // Pop OPTIMIZED_ENTRY_FRAME
    __ Movq(r12, rdx);
    OptimizedCall::PopJSFunctionEntryFrame(assembler, rdx);
    __ Ret();
}

// Global eager-deopt entry used by the fixed five-byte exits installed in C09.
//
// Entry state:
//   r13       = glue
//   rbp       = optimized ArkSteed frame pointer
//   [rsp]     = function-local overflow continuation
//   [rsp + 8] = return PC immediately after the fixed call exit
//
// The entry first snapshots every allocatable register. Only r10 (the entry
// target) and r13 (glue) may be used before the snapshot is complete.
void ArkSteedCall::ArkSteedDeoptimizationEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ArkSteedDeoptimizationEntry));

    constexpr int32_t snapshotSize =
        static_cast<int32_t>(eager_deopt_abi::SNAPSHOT_SIZE);
    constexpr int32_t entryFrameSize =
        static_cast<int32_t>(eager_deopt_abi::ENTRY_FRAME_SIZE);
    constexpr int32_t glueSlotOffset = snapshotSize;
    constexpr int32_t fixedReturnPcOffset = entryFrameSize + FRAME_SLOT_SIZE;

    static_assert(eager_deopt_abi::RETURN_PC_SOURCE ==
                  arksteed::ArkSteedEagerDeoptReturnPcSource::STACK_LINK);
    static_assert(eager_deopt_abi::FIXED_EXIT_LINK_SIZE == FRAME_SLOT_SIZE);
    static_assert(eager_deopt_abi::VENEER_FRAME_SIZE == FRAME_SLOT_SIZE);
    static_assert(entryFrameSize == snapshotSize + DOUBLE_SLOT_SIZE);
    static_assert(entryFrameSize % arksteed::ARKSTEED_EAGER_DEOPT_STACK_ALIGNMENT == 0U);

    Register glue = r13;
    Label stackOverflow;
    Label invalidContext;

    // The fixed exit and veneer calls leave rsp 16-byte aligned. The 224-byte
    // entry frame preserves that alignment at the direct SysV C-call site.
    __ Subq(Immediate(entryFrameSize), rsp);
    for (uint32_t slot = 0; slot < eager_deopt_abi::GENERAL_REGISTER_CODES.size(); ++slot) {
        Register source =
            Register::FromCode(static_cast<int8_t>(eager_deopt_abi::GENERAL_REGISTER_CODES[slot]));
        __ Movq(source, Operand(rsp, static_cast<int32_t>(slot * FRAME_SLOT_SIZE)));
    }
    for (int32_t code = 0;
         code < static_cast<int32_t>(eager_deopt_abi::FLOATING_REGISTER_COUNT); ++code) {
        __ Movsd(Operand(rsp, eager_deopt_abi::FLOATING_SNAPSHOT_OFFSET + code * FRAME_SLOT_SIZE),
                 XMMRegister::FromCode(static_cast<int8_t>(code)));
    }
    __ Movq(glue, Operand(rsp, glueSlotOffset));

    // SysV: ArkSteedDeoptimize(glue, returnPc, inputFp, snapshot).
    __ Movq(glue, rdi);
    __ Movq(Operand(rsp, fixedReturnPcOffset), rsi);
    __ Movq(rbp, rdx);
    __ Movq(rsp, rcx);
    __ Movq(Immediate(RTSTUB_ID(ArkSteedDeoptimize)), r11);
    __ Movq(Operand(rdi, r11, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r11);
    __ Callq(r11);

    __ Cmpq(Immediate(static_cast<int32_t>(arksteed::ArkSteedEagerDeoptResult::STACK_OVERFLOW)), rax);
    __ Je(&stackOverflow);
    __ Cmpq(Immediate(static_cast<int32_t>(arksteed::ArkSteedEagerDeoptResult::INVALID)), rax);
    __ Je(&invalidContext);

    Register context = rsi;
    Register callFrameTop = r10;
    Register sourceReturnAddress = r11;
    __ Movq(rax, context);
    __ Movq(Operand(rsp, glueSlotOffset), rdi);
    __ Movq(Operand(context, AsmStackContext::GetCallerFpOffset(false)), rbp);
    __ Movq(Operand(context, AsmStackContext::GetCallFrameTopOffset(false)), callFrameTop);
    __ Movq(Operand(context, AsmStackContext::GetReturnAddressOffset(false)), sourceReturnAddress);
    __ Movq(sourceReturnAddress, Operand(callFrameTop, -FRAME_SLOT_SIZE));
    __ Movq(callFrameTop, rsp);
    __ Subq(FRAME_SLOT_SIZE, rsp);

    // Reuse the established interpreter/baseline installation protocol.
    Label enterDeoptimizedFrame;
    OptimizedCall::DeoptPushAsmInterpBridgeFrame(assembler, context);
    __ Callq(&enterDeoptimizedFrame);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&enterDeoptimizedFrame);
    OptimizedCall::DeoptEnterAsmInterpOrBaseline(assembler);
    __ Int3();

    __ Bind(&stackOverflow);
    __ Addq(Immediate(entryFrameSize), rsp);
    __ Ret();

    __ Bind(&invalidContext);
    __ Int3();
}

// Entry state for SteedCallAndPushArgv (CCallConv variadic stub):
//   rdi = glue
//   rsi = actualNumArgs(total)
//   rdx = actualArgV (0 for fixed-arg path)
//   rcx = jsFunc
//   r8  = newTarget
//   r9  = this
//   [entry sp + 0] = arg0
//   [entry sp + 8] = arg1
//   ...
//
// This stub extracts callee metadata and delegates to EmitSteedCall, which
// materializes the final Steed frame from C ABI fixed args.
void ArkSteedCall::SteedCallAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SteedCallAndPushArgv));
    Register glue = rdi;
    Register actualNumArgs = rsi;
    Register jsfunc = rcx;
    Register method = r10;
    Register expectedNumArgs = r11;
    Register codeAddr = rax;
    Register newTarget = r8;
    Register thisObj = r9;

    LoadSteedCallTargetInfo(assembler, jsfunc, method, codeAddr, expectedNumArgs);

    EmitSteedCall(assembler, glue, jsfunc, codeAddr, newTarget, thisObj,
                  actualNumArgs, expectedNumArgs,
                  [assembler](Register actualArgc, Register currentSp, Label *invokeSteedCode) {
                      CopyUserArgsFromCCallArgs(assembler, actualArgc, currentSp, invokeSteedCode);
                  });
}

// Entry state for SteedCallWithArgVAndPushArgv (CCallConv variadic stub):
//   rdi = glue
//   rsi = actualNumArgs(user argc only)
//   rdx = jsFunc (call-target)
//   rcx = newTarget
//   r8  = this
//   r9  = argV (pointer to user args [arg0, arg1, ...])
//
// This stub converts user argc to total argc, then delegates to EmitSteedCall
// with argV-based user-arg copying.
void ArkSteedCall::SteedCallWithArgVAndPushArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SteedCallWithArgVAndPushArgv));
    Register glue = rdi;
    Register jsfunc = rdx;
    Register method = r10;
    Register expectedNumArgs = r11;
    Register codeAddr = rax;
    Register argV = r9;
    Register newTarget = rcx;
    Register thisObj = r8;
    Register actualNumArgs = rsi;

    __ Addq(NUM_MANDATORY_JSFUNC_ARGS, actualNumArgs);
    LoadSteedCallTargetInfo(assembler, jsfunc, method, codeAddr, expectedNumArgs);

    EmitSteedCall(assembler, glue, jsfunc, codeAddr, newTarget, thisObj,
                  actualNumArgs, expectedNumArgs,
                  [assembler, glue, argV](Register actualArgc, Register currentSp, Label *invokeSteedCode) {
                      CopyUserArgsFromArgV(assembler, glue, actualArgc, argV, currentSp, invokeSteedCode);
                  });
}

#undef __
}  // namespace panda::ecmascript::x64
