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
#include "ecmascript/method.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
#define __ assembler->

// Generate code for entering asm interpreter
// c++ calling convention
// Input: glue           - %X0
//        callTarget     - %X1
//        method         - %X2
//        callField      - %X3
//        argc           - %X4
//        argv           - %X5(<callTarget, newTarget, this> are at the beginning of argv)
void AsmInterpreterCall::AsmInterpreterEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(AsmInterpreterEntry));
    Label target;
    size_t begin = __ GetCurrentPosition();
    PushAsmInterpEntryFrame(assembler);
    __ Bl(&target);
    PopAsmInterpEntryFrame(assembler);
    size_t end = __ GetCurrentPosition();
    if ((end - begin) != FrameCompletionPos::ARM64EntryFrameDuration) {
        LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::ARM64EntryFrameDuration
                            << "This frame has been modified, and the offset EntryFrameDuration should be updated too.";
    }
    __ Ret();

    __ Bind(&target);
    {
        AsmInterpEntryDispatch(assembler);
    }
}

// Input: glue           - %X0
//        callTarget     - %X1
//        method         - %X2
//        callField      - %X3
//        argc           - %X4
//        argv           - %X5(<callTarget, newTarget, this> are at the beginning of argv)
void AsmInterpreterCall::AsmInterpEntryDispatch(ExtendedAssembler *assembler)
{
    Label notJSFunction;
    Label callNativeEntry;
    Label callJSFunctionEntry;
    Label notCallable;
    Register glueRegister(X0);
    Register argcRegister(X4, W);
    Register argvRegister(X5);
    Register callTargetRegister(X1);
    Register callFieldRegister(X3);
    Register bitFieldRegister(X16);
    Register tempRegister(X17); // can not be used to store any variable
    Register functionTypeRegister(X18, W);
    __ Ldr(tempRegister, MemoryOperand(callTargetRegister, TaggedObject::HCLASS_OFFSET));
    __ Ldr(bitFieldRegister, MemoryOperand(tempRegister, JSHClass::BIT_FIELD_OFFSET));
    __ And(functionTypeRegister, bitFieldRegister.W(), LogicalImmediate::Create(0xFF, RegWSize));
    __ Mov(tempRegister.W(), Immediate(static_cast<int64_t>(JSType::JS_FUNCTION_FIRST)));
    __ Cmp(functionTypeRegister, tempRegister.W());
    __ B(Condition::LO, &notJSFunction);
    __ Mov(tempRegister.W(), Immediate(static_cast<int64_t>(JSType::JS_FUNCTION_LAST)));
    __ Cmp(functionTypeRegister, tempRegister.W());
    __ B(Condition::LS, &callJSFunctionEntry);
    __ Bind(&notJSFunction);
    {
        __ Tst(bitFieldRegister,
            LogicalImmediate::Create(static_cast<int64_t>(1ULL << JSHClass::CallableBit::START_BIT), RegXSize));
        __ B(Condition::EQ, &notCallable);
        // fall through
    }
    __ Bind(&callNativeEntry);
    CallNativeEntry(assembler);
    __ Bind(&callJSFunctionEntry);
    {
        __ Tbnz(callFieldRegister, MethodLiteral::IsNativeBit::START_BIT, &callNativeEntry);
        // fast path
        __ Add(argvRegister, argvRegister, Immediate(NUM_MANDATORY_JSFUNC_ARGS * JSTaggedValue::TaggedTypeSize()));
        JSCallCommonEntry(assembler, JSCallMode::CALL_ENTRY);
    }
    __ Bind(&notCallable);
    {
        Register runtimeId(X11);
        Register trampoline(X12);
        __ Mov(runtimeId, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowNotCallableException));
        // 3 : 3 means *8
        __ Add(trampoline, glueRegister, Operand(runtimeId, LSL, 3));
        __ Ldr(trampoline, MemoryOperand(trampoline, JSThread::GlueData::GetRTStubEntriesOffset(false)));
        __ Blr(trampoline);
        __ Ret();
    }
}

void AsmInterpreterCall::JSCallCommonEntry(ExtendedAssembler *assembler, JSCallMode mode)
{
    Label stackOverflow;
    Register glueRegister = __ GlueRegister();
    Register fpRegister = __ AvailableRegister1();
    Register currentSlotRegister = __ AvailableRegister3();
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
    if (!kungfu::AssemblerModule::IsJumpToCallCommonEntry(mode)) {
        __ PushFpAndLr();
    }
    // save fp
    __ Mov(fpRegister, Register(SP));
    __ Mov(currentSlotRegister, Register(SP));

    {
        // Reserve enough sp space to prevent stack parameters from being covered by cpu profiler.
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register tempRegister = __ TempRegister1();
        __ Ldr(tempRegister, MemoryOperand(glueRegister, JSThread::GlueData::GetCurrentContextOffset(false)));
        __ Ldr(tempRegister, MemoryOperand(tempRegister, 0));
        __ Mov(Register(SP), tempRegister);
    }

    Register declaredNumArgsRegister = __ AvailableRegister2();
    GetDeclaredNumArgsFromCallField(assembler, callFieldRegister, declaredNumArgsRegister);

    Label slowPathEntry;
    Label fastPathEntry;
    Label pushCallThis;
    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    if (argc >= 0) {
        __ Cmp(declaredNumArgsRegister, Immediate(argc));
    } else {
        __ Cmp(declaredNumArgsRegister, argcRegister);
    }
    __ B(Condition::NE, &slowPathEntry);
    __ Bind(&fastPathEntry);
    JSCallCommonFastPath(assembler, mode, &pushCallThis, &stackOverflow);
    __ Bind(&pushCallThis);
    PushCallThis(assembler, mode, &stackOverflow);
    __ Bind(&slowPathEntry);
    JSCallCommonSlowPath(assembler, mode, &fastPathEntry, &pushCallThis, &stackOverflow);

    __ Bind(&stackOverflow);
    if (kungfu::AssemblerModule::IsJumpToCallCommonEntry(mode)) {
        __ Mov(Register(SP), fpRegister);
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register temp = __ TempRegister1();
        // only glue and acc are useful in exception handler
        if (glueRegister.GetId() != X19) {
            __ Mov(Register(X19), glueRegister);
        }
        Register acc(X23);
        __ Mov(acc, Immediate(JSTaggedValue::VALUE_EXCEPTION));
        Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);
        Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
        // Reload pc to make sure stack trace is right
        __ Mov(temp, callTargetRegister);
        __ Ldr(Register(X20), MemoryOperand(methodRegister, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        // Reload constpool and profileInfo to make sure gc map work normally
        __ Ldr(Register(X22), MemoryOperand(methodRegister, Method::PROFILE_TYPE_INFO_OFFSET));
        __ Ldr(Register(X21), MemoryOperand(methodRegister, Method::CONSTANT_POOL_OFFSET));

        __ Mov(temp, kungfu::BytecodeStubCSigns::ID_ThrowStackOverflowException);
        __ Add(temp, glueRegister, Operand(temp, UXTW, 3));  // 3： bc * 8
        __ Ldr(temp, MemoryOperand(temp, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(temp);
    } else {
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register temp = __ TempRegister1();
        ThrowStackOverflowExceptionAndReturn(assembler, glueRegister, fpRegister, temp);
    }
}

void AsmInterpreterCall::JSCallCommonFastPath(ExtendedAssembler *assembler, JSCallMode mode, Label *pushCallThis,
    Label *stackOverflow)
{
    Register glueRegister = __ GlueRegister();
    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    Register currentSlotRegister = __ AvailableRegister3();
    // call range
    if (argc < 0) {
        Register numRegister = __ AvailableRegister2();
        Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
        Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGV);
        __ Mov(numRegister, argcRegister);
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register opRegister = __ TempRegister1();
        PushArgsWithArgv(assembler, glueRegister, numRegister, argvRegister, opRegister,
                         currentSlotRegister, pushCallThis, stackOverflow);
    } else if (argc > 0) {
        if (argc > 2) { // 2: call arg2
            Register arg2 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
            __ Str(arg2, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
        if (argc > 1) {
            Register arg1 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
            __ Str(arg1, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
        if (argc > 0) {
            Register arg0 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
            __ Str(arg0, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
    }
}

void AsmInterpreterCall::JSCallCommonSlowPath(ExtendedAssembler *assembler, JSCallMode mode,
                                              Label *fastPathEntry, Label *pushCallThis, Label *stackOverflow)
{
    Register glueRegister = __ GlueRegister();
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
    Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGV);
    Register currentSlotRegister = __ AvailableRegister3();
    Register arg0 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
    Register arg1 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
    Label noExtraEntry;
    Label pushArgsEntry;

    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    Register declaredNumArgsRegister = __ AvailableRegister2();
    __ Tbz(callFieldRegister, MethodLiteral::HaveExtraBit::START_BIT, &noExtraEntry);
    // extra entry
    {
        [[maybe_unused]] TempRegister1Scope scope1(assembler);
        Register tempArgcRegister = __ TempRegister1();
        if (argc >= 0) {
            __ PushArgc(argc, tempArgcRegister, currentSlotRegister);
        } else {
            __ PushArgc(argcRegister, tempArgcRegister, currentSlotRegister);
        }
        // fall through
    }
    __ Bind(&noExtraEntry);
    {
        if (argc == 0) {
            {
                [[maybe_unused]] TempRegister1Scope scope(assembler);
                Register tempRegister = __ TempRegister1();
                PushUndefinedWithArgc(assembler, glueRegister, declaredNumArgsRegister, tempRegister,
                                      currentSlotRegister, nullptr, stackOverflow);
            }
            __ B(fastPathEntry);
            return;
        }
        [[maybe_unused]] TempRegister1Scope scope1(assembler);
        Register diffRegister = __ TempRegister1();
        if (argc >= 0) {
            __ Sub(diffRegister.W(), declaredNumArgsRegister.W(), Immediate(argc));
        } else {
            __ Sub(diffRegister.W(), declaredNumArgsRegister.W(), argcRegister.W());
        }
        [[maybe_unused]] TempRegister2Scope scope2(assembler);
        Register tempRegister = __ TempRegister2();
        PushUndefinedWithArgc(assembler, glueRegister, diffRegister, tempRegister,
                              currentSlotRegister, &pushArgsEntry, stackOverflow);
        __ B(fastPathEntry);
    }
    // declare < actual
    __ Bind(&pushArgsEntry);
    {
        __ Tbnz(callFieldRegister, MethodLiteral::HaveExtraBit::START_BIT, fastPathEntry);
        // no extra branch
        // arg1, declare must be 0
        if (argc == 1) {
            __ B(pushCallThis);
            return;
        }
        __ Cmp(declaredNumArgsRegister, Immediate(0));
        __ B(Condition::EQ, pushCallThis);
        // call range
        if (argc < 0) {
            [[maybe_unused]] TempRegister1Scope scope(assembler);
            Register opRegister = __ TempRegister1();
            PushArgsWithArgv(assembler, glueRegister, declaredNumArgsRegister,
                             argvRegister, opRegister,
                             currentSlotRegister, nullptr, stackOverflow);
        } else if (argc > 0) {
            Label pushArgs0;
            if (argc > 2) {  // 2: call arg2
                // decalare is 2 or 1 now
                __ Cmp(declaredNumArgsRegister, Immediate(1));
                __ B(Condition::EQ, &pushArgs0);
                __ Str(arg1, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
            }
            if (argc > 1) {
                __ Bind(&pushArgs0);
                // decalare is is 1 now
                __ Str(arg0, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
            }
        }
        __ B(pushCallThis);
    }
}

Register AsmInterpreterCall::GetThisRegsiter(ExtendedAssembler *assembler, JSCallMode mode, Register defaultRegister)
{
    switch (mode) {
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_THIS_ARG0:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG1:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG3);
        case JSCallMode::CALL_FROM_AOT:
        case JSCallMode::CALL_ENTRY: {
            Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
            __ Ldur(defaultRegister, MemoryOperand(argvRegister, -FRAME_SLOT_SIZE));
            return defaultRegister;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return INVALID_REG;
}

Register AsmInterpreterCall::GetNewTargetRegsiter(ExtendedAssembler *assembler, JSCallMode mode,
    Register defaultRegister)
{
    switch (mode) {
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
        case JSCallMode::CALL_FROM_AOT:
        case JSCallMode::CALL_ENTRY: {
            Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
            // 2: new Target index
            __ Ldur(defaultRegister, MemoryOperand(argvRegister, -2 * FRAME_SLOT_SIZE));
            return defaultRegister;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return INVALID_REG;
}

// void PushCallArgsxAndDispatch(uintptr_t glue, uintptr_t sp, uint64_t callTarget, uintptr_t method,
//     uint64_t callField, ...)
// GHC calling convention
// Input1: for callarg0/1/2/3        Input2: for callrange
// X19 - glue                        // X19 - glue
// FP  - sp                          // FP  - sp
// X20 - callTarget                  // X20 - callTarget
// X21 - method                      // X21 - method
// X22 - callField                   // X22 - callField
// X23 - arg0                        // X23 - actualArgc
// X24 - arg1                        // X24 - argv
// X25 - arg2
void AsmInterpreterCall::PushCallThisRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisRangeAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_WITH_ARGV);
}

void AsmInterpreterCall::PushCallRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallRangeAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_WITH_ARGV);
}

void AsmInterpreterCall::PushCallNewAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallNewAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV);
}

void AsmInterpreterCall::PushCallArgs3AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs3AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG3);
}

void AsmInterpreterCall::PushCallArgs2AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs2AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG2);
}

void AsmInterpreterCall::PushCallArg1AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArg1AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG1);
}

void AsmInterpreterCall::PushCallArg0AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArg0AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG0);
}

void AsmInterpreterCall::PushCallThisArg0AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisArg0AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG0);
}

void AsmInterpreterCall::PushCallThisArg1AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisArg1AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG1);
}

void AsmInterpreterCall::PushCallThisArgs2AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisArgs2AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG2);
}

void AsmInterpreterCall::PushCallThisArgs3AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisArgs3AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG3);
}

// uint64_t PushCallRangeAndDispatchNative(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, uintptr_t argv[])
// c++ calling convention call js function
// Input: X0 - glue
//        X1 - nativeCode
//        X2 - callTarget
//        X3 - thisValue
//        X4  - argc
//        X5  - argV (...)
void AsmInterpreterCall::PushCallRangeAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallRangeAndDispatchNative));
    CallNativeWithArgv(assembler, false);
}

void AsmInterpreterCall::PushCallNewAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallNewAndDispatchNative));
    CallNativeWithArgv(assembler, true);
}

void AsmInterpreterCall::CallNativeWithArgv(ExtendedAssembler *assembler, bool callNew)
{
    Register glue(X0);
    Register nativeCode(X1);
    Register callTarget(X2);
    Register thisObj(X3);
    Register argc(X4);
    Register argv(X5);
    Register opArgc(X8);
    Register opArgv(X9);
    Register temp(X10);
    Register currentSlotRegister(X11);
    Register spRegister(SP);

    Label pushThis;
    Label stackOverflow;
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME_WITH_ARGV, temp, argc);

    __ Mov(currentSlotRegister, spRegister);
    // Reserve enough sp space to prevent stack parameters from being covered by cpu profiler.
    __ Ldr(temp, MemoryOperand(glue, JSThread::GlueData::GetCurrentContextOffset(false)));
    __ Ldr(temp, MemoryOperand(temp, 0));
    __ Mov(Register(SP), temp);

    __ Mov(opArgc, argc);
    __ Mov(opArgv, argv);
    PushArgsWithArgv(assembler, glue, opArgc, opArgv, temp, currentSlotRegister, &pushThis, &stackOverflow);

    __ Bind(&pushThis);
    // newTarget
    if (callNew) {
        // 16: this & newTarget
        __ Stp(callTarget, thisObj, MemoryOperand(currentSlotRegister, -16, AddrMode::PREINDEX));
    } else {
        __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        // 16: this & newTarget
        __ Stp(temp, thisObj, MemoryOperand(currentSlotRegister, -16, AddrMode::PREINDEX));
    }
    // callTarget
    __ Str(callTarget, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(temp, currentSlotRegister, Immediate(40));  // 40: skip frame type, numArgs, func, newTarget and this
    __ Add(Register(FP), temp, Operand(argc, LSL, 3));  // 3: argc * 8

    __ Add(temp, argc, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    // 2: thread & argc
    __ Stp(glue, temp, MemoryOperand(currentSlotRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(Register(X0), currentSlotRegister, Immediate(0));

    __ Align16(currentSlotRegister);
    __ Mov(spRegister, currentSlotRegister);

    CallNativeInternal(assembler, nativeCode);
    __ Ret();

    __ Bind(&stackOverflow);
    {
        // use builtin_with_argv_frame to mark gc map
        Register frameType(X11);
        __ Ldr(temp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
        __ Mov(spRegister, temp);
        __ Mov(frameType, Immediate(static_cast<int32_t>(FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME)));
        // 2: frame type and argc
        __ Stp(Register(Zero), frameType, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        // 2: fill this&newtgt slots
        __ Stp(temp, temp, MemoryOperand(spRegister, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        // 2: fill func&align slots
        __ Stp(Register(Zero), temp, MemoryOperand(spRegister, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));

        Register runtimeId(X11);
        Register trampoline(X12);
        __ Mov(runtimeId, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException));
        // 3 : 3 means *8
        __ Add(trampoline, glue, Operand(runtimeId, LSL, 3));
        __ Ldr(trampoline, MemoryOperand(trampoline, JSThread::GlueData::GetRTStubEntriesOffset(false)));
        __ Blr(trampoline);

        // resume rsp
        __ Mov(Register(SP), Register(FP));
        __ RestoreFpAndLr();
        __ Ret();
    }
}

// uint64_t PushCallArgsAndDispatchNative(uintptr_t codeAddress, uintptr_t glue, uint32_t argc, ...)
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input: X0 - codeAddress
// stack layout: sp + N*8 argvN
//               ........
//               sp + 24: argv1
//               sp + 16: argv0
//               sp + 8:  actualArgc
//               sp:      thread
// construct Native Leave Frame
//               +--------------------------+
//               |     argV[N - 1]          |
//               |--------------------------|
//               |       . . . .            |
//               |--------------------------+
//               |     argV[2]=this         |
//               +--------------------------+
//               |     argV[1]=new-target   |
//               +--------------------------+
//               |     argV[0]=call-target  |
//               +--------------------------+ ---------
//               |       argc               |         ^
//               |--------------------------|         |
//               |       thread             |         |
//               |--------------------------|         |
//               |       returnAddr         |     BuiltinFrame
//               |--------------------------|         |
//               |       callsiteFp         |         |
//               |--------------------------|         |
//               |       frameType          |         v
//               +--------------------------+ ---------

void AsmInterpreterCall::PushCallArgsAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgsAndDispatchNative));

    Register nativeCode(X0);
    Register glue(X1);
    Register argv(X5);
    Register temp(X6);
    Register sp(SP);
    Register nativeCodeTemp(X2);

    __ Mov(nativeCodeTemp, nativeCode);

    __ Ldr(glue, MemoryOperand(sp, 0));
    __ Add(Register(X0), sp, Immediate(0));
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME, temp, argv);

    CallNativeInternal(assembler, nativeCodeTemp);
    __ Ret();
}

void AsmInterpreterCall::PushBuiltinFrame(ExtendedAssembler *assembler, Register glue,
    FrameType type, Register op, Register next)
{
    Register sp(SP);
    __ PushFpAndLr();
    __ Mov(op, sp);
    __ Str(op, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Mov(op, Immediate(static_cast<int32_t>(type)));
    if (type == FrameType::BUILTIN_FRAME) {
        // push stack args
        __ Add(next, sp, Immediate(BuiltinFrame::GetStackArgsToFpDelta(false)));
        // 16: type & next
        __ Stp(next, op, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ Add(Register(FP), sp, Immediate(2 * FRAME_SLOT_SIZE));  // 16: skip next and frame type
    } else {
        // 16: type & next
        __ Stp(next, op, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ Add(Register(FP), sp, Immediate(2 * FRAME_SLOT_SIZE));  // 16: skip next and frame type
    }
}

void AsmInterpreterCall::CallNativeInternal(ExtendedAssembler *assembler, Register nativeCode)
{
    __ Blr(nativeCode);
    // resume rsp
    __ Mov(Register(SP), Register(FP));
    __ RestoreFpAndLr();
}

// ResumeRspAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// X19 - glue
// FP  - sp
// X20 - pc
// X21 - constantPool
// X22 - profileTypeInfo
// X23 - acc
// X24 - hotnessCounter
// X25 - jumpSizeAfterCall
void AsmInterpreterCall::ResumeRspAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndDispatch));

    Register glueRegister = __ GlueRegister();
    Register sp(FP);
    Register rsp(SP);
    Register pc(X20);
    Register jumpSizeRegister(X25);

    Register ret(X23);
    Register opcode(X6, W);
    Register temp(X7);
    Register bcStub(X7);
    Register fp(X8);

    int64_t fpOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    int64_t spOffset = static_cast<int64_t>(AsmInterpretedFrame::GetBaseOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    int64_t thisOffset = static_cast<int64_t>(AsmInterpretedFrame::GetThisOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(fpOffset < 0);
    ASSERT(spOffset < 0);

    Label newObjectRangeReturn;
    Label dispatch;
    __ Ldur(fp, MemoryOperand(sp, fpOffset));  // store fp for temporary
    __ Cmp(jumpSizeRegister, Immediate(0));
    __ B(Condition::LE, &newObjectRangeReturn);
    __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp

    __ Add(pc, pc, Operand(jumpSizeRegister, LSL, 0));
    __ Ldrb(opcode, MemoryOperand(pc, 0));
    __ Bind(&dispatch);
    {
        __ Mov(rsp, fp);  // resume rsp
        __ Add(bcStub, glueRegister, Operand(opcode, UXTW, FRAME_SLOT_SIZE_LOG2));
        __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(bcStub);
    }

    Label getThis;
    Label notUndefined;
    __ Bind(&newObjectRangeReturn);
    {
        __ Cmp(ret, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(Condition::NE, &notUndefined);
        ASSERT(thisOffset < 0);
        __ Bind(&getThis);
        __ Ldur(ret, MemoryOperand(sp, thisOffset));  // update acc
        __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp
        __ Mov(rsp, fp);  // resume rsp
        __ Sub(pc, pc, jumpSizeRegister); // sub negative jmupSize
        __ Ldrb(opcode, MemoryOperand(pc, 0));
        __ Add(bcStub, glueRegister, Operand(opcode, UXTW, FRAME_SLOT_SIZE_LOG2));
        __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(bcStub);
    }
    __ Bind(&notUndefined);
    {
        Label notEcmaObject;
        __ Mov(temp, Immediate(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ And(temp, temp, ret);
        __ Cmp(temp, Immediate(0));
        __ B(Condition::NE, &notEcmaObject);
        // acc is heap object
        __ Ldr(temp, MemoryOperand(ret, TaggedObject::HCLASS_OFFSET));
        __ Ldr(temp, MemoryOperand(temp, JSHClass::BIT_FIELD_OFFSET));
        __ And(temp.W(), temp.W(), LogicalImmediate::Create(0xFF, RegWSize));
        __ Cmp(temp.W(), Immediate(static_cast<int64_t>(JSType::ECMA_OBJECT_LAST)));
        __ B(Condition::HI, &notEcmaObject);
        __ Cmp(temp.W(), Immediate(static_cast<int64_t>(JSType::ECMA_OBJECT_FIRST)));
        __ B(Condition::LO, &notEcmaObject);
        // acc is ecma object
        __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp
        __ Sub(pc, pc, jumpSizeRegister); // sub negative jmupSize
        __ Ldrb(opcode, MemoryOperand(pc, 0));
        __ B(&dispatch);

        __ Bind(&notEcmaObject);
        {
            int64_t constructorOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFunctionOffset(false))
                - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
            ASSERT(constructorOffset < 0);
            __ Ldur(temp, MemoryOperand(sp, constructorOffset));  // load constructor
            __ Ldr(temp, MemoryOperand(temp, JSFunctionBase::METHOD_OFFSET));
            __ Ldr(temp, MemoryOperand(temp, Method::EXTRA_LITERAL_INFO_OFFSET));
            __ Lsr(temp.W(), temp.W(), MethodLiteral::FunctionKindBits::START_BIT);
            __ And(temp.W(), temp.W(),
                LogicalImmediate::Create((1LU << MethodLiteral::FunctionKindBits::SIZE) - 1, RegWSize));
            __ Cmp(temp.W(), Immediate(static_cast<int64_t>(FunctionKind::CLASS_CONSTRUCTOR)));
            __ B(Condition::LS, &getThis);  // constructor is base
            // exception branch
            {
                __ Mov(opcode, kungfu::BytecodeStubCSigns::ID_NewObjectRangeThrowException);
                __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp
                __ B(&dispatch);
            }
        }
    }
}

// ResumeRspAndReturn(uintptr_t acc)
// GHC calling convention
// X19 - acc
// FP - prevSp
// X20 - sp
void AsmInterpreterCall::ResumeRspAndReturn(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndReturn));
    Register rsp(SP);
    Register currentSp(X20);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register fpRegister = __ TempRegister1();
    int64_t offset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(offset < 0);
    __ Ldur(fpRegister, MemoryOperand(currentSp, offset));
    __ Mov(rsp, fpRegister);

    // return
    {
        __ RestoreFpAndLr();
        __ Mov(Register(X0), Register(X19));
        __ Ret();
    }
}

// ResumeCaughtFrameAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter)
// GHC calling convention
// X19 - glue
// FP  - sp
// X20 - pc
// X21 - constantPool
// X22 - profileTypeInfo
// X23 - acc
// X24 - hotnessCounter
void AsmInterpreterCall::ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeCaughtFrameAndDispatch));

    Register glue(X19);
    Register pc(X20);
    Register fp(X5);
    Register opcode(X6, W);
    Register bcStub(X7);

    Label dispatch;
    __ Ldr(fp, MemoryOperand(glue, JSThread::GlueData::GetLastFpOffset(false)));
    __ Cmp(fp, Immediate(0));
    __ B(Condition::EQ, &dispatch);
    // up frame
    __ Mov(Register(SP), fp);
    // fall through
    __ Bind(&dispatch);
    {
        __ Ldrb(opcode, MemoryOperand(pc, 0));
        __ Add(bcStub, glue, Operand(opcode, UXTW, FRAME_SLOT_SIZE_LOG2));
        __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(bcStub);
    }
}

// ResumeUncaughtFrameAndReturn(uintptr_t glue)
// GHC calling convention
// X19 - glue
// FP - sp
// X20 - acc
void AsmInterpreterCall::ResumeUncaughtFrameAndReturn(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeUncaughtFrameAndReturn));

    Register glue(X19);
    Register fp(X5);
    Register acc(X20);
    Register cppRet(X0);

    __ Ldr(fp, MemoryOperand(glue, JSThread::GlueData::GetLastFpOffset(false)));
    __ Mov(Register(SP), fp);
    // this method will return to Execute(cpp calling convention), and the return value should be put into X0.
    __ Mov(cppRet, acc);
    __ RestoreFpAndLr();
    __ Ret();
}

// ResumeRspAndRollback(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// X19 - glue
// FP  - sp
// X20 - pc
// X21 - constantPool
// X22 - profileTypeInfo
// X23 - acc
// X24 - hotnessCounter
// X25 - jumpSizeAfterCall
void AsmInterpreterCall::ResumeRspAndRollback(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndRollback));

    Register glueRegister = __ GlueRegister();
    Register sp(FP);
    Register rsp(SP);
    Register pc(X20);
    Register jumpSizeRegister(X25);

    Register ret(X23);
    Register opcode(X6, W);
    Register bcStub(X7);
    Register fp(X8);

    int64_t fpOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    int64_t spOffset = static_cast<int64_t>(AsmInterpretedFrame::GetBaseOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    int64_t funcOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFunctionOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(fpOffset < 0);
    ASSERT(spOffset < 0);
    ASSERT(funcOffset < 0);

    __ Ldur(fp, MemoryOperand(sp, fpOffset));  // store fp for temporary
    __ Ldur(ret, MemoryOperand(sp, funcOffset)); // restore acc
    __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp

    __ Add(pc, pc, Operand(jumpSizeRegister, LSL, 0));
    __ Ldrb(opcode, MemoryOperand(pc, 0));

    __ Mov(rsp, fp);  // resume rsp
    __ Add(bcStub, glueRegister, Operand(opcode, UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(bcStub);
}

// c++ calling convention
// X0 - glue
// X1 - callTarget
// X2 - method
// X3 - callField
// X4 - receiver
// X5 - value
void AsmInterpreterCall::CallGetter(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallGetter));
    Label target;

    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_GETTER);
    }
}

void AsmInterpreterCall::CallSetter(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallSetter));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_SETTER);
    }
}

void AsmInterpreterCall::CallContainersArgs3(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallContainersArgs3));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
    }
}

// c++ calling convention
// X0 - glue
// X1 - callTarget
// X2 - method
// X3 - callField
// X4 - arg0(argc)
// X5 - arg1(arglist)
// X6 - arg3(argthis)
void AsmInterpreterCall::CallReturnWithArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallReturnWithArgv));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
    }
}

// Generate code for generator re-entering asm interpreter
// c++ calling convention
// Input: %X0 - glue
//        %X1 - context(GeneratorContext)
void AsmInterpreterCall::GeneratorReEnterAsmInterp(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(GeneratorReEnterAsmInterp));
    Label target;
    size_t begin = __ GetCurrentPosition();
    PushAsmInterpEntryFrame(assembler);
    __ Bl(&target);
    PopAsmInterpEntryFrame(assembler);
    size_t end = __ GetCurrentPosition();
    if ((end - begin) != FrameCompletionPos::ARM64EntryFrameDuration) {
        LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::ARM64EntryFrameDuration
                            << "This frame has been modified, and the offset EntryFrameDuration should be updated too.";
    }
    __ Ret();
    __ Bind(&target);
    {
        GeneratorReEnterAsmInterpDispatch(assembler);
    }
}

void AsmInterpreterCall::GeneratorReEnterAsmInterpDispatch(ExtendedAssembler *assembler)
{
    Label pushFrameState;
    Label stackOverflow;
    Register glue = __ GlueRegister();
    Register contextRegister(X1);
    Register spRegister(SP);
    Register pc(X8);
    Register prevSpRegister(FP);
    Register callTarget(X4);
    Register method(X5);
    Register temp(X6); // can not be used to store any variable
    Register currentSlotRegister(X7);
    Register fpRegister(X9);
    Register thisRegister(X25);
    Register nRegsRegister(X26, W);
    Register regsArrayRegister(X27);
    Register newSp(X28);
    __ Ldr(callTarget, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_METHOD_OFFSET));
    __ Ldr(method, MemoryOperand(callTarget, JSFunctionBase::METHOD_OFFSET));
    __ PushFpAndLr();
    // save fp
    __ Mov(fpRegister, spRegister);
    __ Mov(currentSlotRegister, spRegister);
    // Reserve enough sp space to prevent stack parameters from being covered by cpu profiler.
    __ Ldr(temp, MemoryOperand(glue, JSThread::GlueData::GetCurrentContextOffset(false)));
    __ Ldr(temp, MemoryOperand(temp, 0));
    __ Mov(Register(SP), temp);
    // push context regs
    __ Ldr(nRegsRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_NREGS_OFFSET));
    __ Ldr(thisRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_THIS_OFFSET));
    __ Ldr(regsArrayRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET));
    __ Add(regsArrayRegister, regsArrayRegister, Immediate(TaggedArray::DATA_OFFSET));
    PushArgsWithArgv(assembler, glue, nRegsRegister, regsArrayRegister, temp,
                     currentSlotRegister, &pushFrameState, &stackOverflow);

    __ Bind(&pushFrameState);
    __ Mov(newSp, currentSlotRegister);
    // push frame state
    PushGeneratorFrameState(assembler, prevSpRegister, fpRegister, currentSlotRegister, callTarget, thisRegister,
                            method, contextRegister, pc, temp);
    __ Align16(currentSlotRegister);
    __ Mov(Register(SP), currentSlotRegister);
    // call bc stub
    CallBCStub(assembler, newSp, glue, method, pc, temp);

    __ Bind(&stackOverflow);
    {
        ThrowStackOverflowExceptionAndReturn(assembler, glue, fpRegister, temp);
    }
}

void AsmInterpreterCall::PushCallThis(ExtendedAssembler *assembler, JSCallMode mode, Label *stackOverflow)
{
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register thisRegister = __ AvailableRegister2();
    Register currentSlotRegister = __ AvailableRegister3();

    Label pushVregs;
    Label pushNewTarget;
    Label pushCallTarget;
    bool haveThis = kungfu::AssemblerModule::JSModeHaveThisArg(mode);
    bool haveNewTarget = kungfu::AssemblerModule::JSModeHaveNewTargetArg(mode);
    if (!haveThis) {
        __ Mov(thisRegister, Immediate(JSTaggedValue::VALUE_UNDEFINED));  // default this: undefined
    } else {
        Register thisArgRegister = GetThisRegsiter(assembler, mode, thisRegister);
        if (thisRegister.GetId() != thisArgRegister.GetId()) {
            __ Mov(thisRegister, thisArgRegister);
        }
    }
    __ Tst(callFieldRegister, LogicalImmediate::Create(CALL_TYPE_MASK, RegXSize));
    __ B(Condition::EQ, &pushVregs);
    __ Tbz(callFieldRegister, MethodLiteral::HaveThisBit::START_BIT, &pushNewTarget);
    if (!haveThis) {
        [[maybe_unused]] TempRegister1Scope scope1(assembler);
        Register tempRegister = __ TempRegister1();
        __ Mov(tempRegister, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Str(tempRegister, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    } else {
        __ Str(thisRegister, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ Bind(&pushNewTarget);
    {
        __ Tbz(callFieldRegister, MethodLiteral::HaveNewTargetBit::START_BIT, &pushCallTarget);
        if (!haveNewTarget) {
            [[maybe_unused]] TempRegister1Scope scope1(assembler);
            Register newTarget = __ TempRegister1();
            __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
            __ Str(newTarget, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        } else {
            [[maybe_unused]] TempRegister1Scope scope1(assembler);
            Register defaultRegister = __ TempRegister1();
            Register newTargetRegister = GetNewTargetRegsiter(assembler, mode, defaultRegister);
            __ Str(newTargetRegister, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
    }
    __ Bind(&pushCallTarget);
    {
        __ Tbz(callFieldRegister, MethodLiteral::HaveFuncBit::START_BIT, &pushVregs);
        __ Str(callTargetRegister, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ Bind(&pushVregs);
    {
        PushVregs(assembler, stackOverflow);
    }
}

void AsmInterpreterCall::PushVregs(ExtendedAssembler *assembler, Label *stackOverflow)
{
    Register glue = __ GlueRegister();
    Register prevSpRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::SP);
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register fpRegister = __ AvailableRegister1();
    Register thisRegister = __ AvailableRegister2();
    Register currentSlotRegister = __ AvailableRegister3();

    Label pushFrameStateAndCall;
    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register tempRegister = __ TempRegister1();
    // args register can be reused now.
    Register newSpRegister = __ AvailableRegister4();
    Register numVregsRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
    GetNumVregsFromCallField(assembler, callFieldRegister, numVregsRegister);
    PushUndefinedWithArgc(assembler, glue, numVregsRegister, tempRegister, currentSlotRegister, &pushFrameStateAndCall,
        stackOverflow);
    // fall through
    __ Bind(&pushFrameStateAndCall);
    {
        __ Mov(newSpRegister, currentSlotRegister);

        [[maybe_unused]] TempRegister2Scope scope2(assembler);
        Register pcRegister = __ TempRegister2();
        PushFrameState(assembler, prevSpRegister, fpRegister, currentSlotRegister, callTargetRegister, thisRegister,
            methodRegister, pcRegister, tempRegister);

        __ Align16(currentSlotRegister);
        __ Mov(Register(SP), currentSlotRegister);
        DispatchCall(assembler, pcRegister, newSpRegister);
    }
}

// Input: X19 - glue
//        FP - sp
//        X20 - callTarget
//        X21 - method
void AsmInterpreterCall::DispatchCall(ExtendedAssembler *assembler, Register pcRegister,
    Register newSpRegister, Register accRegister)
{
    Register glueRegister = __ GlueRegister();
    Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);

    if (glueRegister.GetId() != X19) {
        __ Mov(Register(X19), glueRegister);
    }
    __ Ldrh(Register(X24, W), MemoryOperand(methodRegister, Method::LITERAL_INFO_OFFSET));
    if (accRegister == INVALID_REG) {
        __ Mov(Register(X23), Immediate(JSTaggedValue::VALUE_HOLE));
    } else {
        ASSERT(accRegister == Register(X23));
    }
    __ Ldr(Register(X22), MemoryOperand(methodRegister, Method::PROFILE_TYPE_INFO_OFFSET));
    __ Ldr(Register(X21), MemoryOperand(methodRegister, Method::CONSTANT_POOL_OFFSET));
    __ Mov(Register(X20), pcRegister);
    __ Mov(Register(FP), newSpRegister);

    Register bcIndexRegister = __ AvailableRegister1();
    Register tempRegister = __ AvailableRegister2();
    __ Ldrb(bcIndexRegister.W(), MemoryOperand(pcRegister, 0));
    __ Add(tempRegister, glueRegister, Operand(bcIndexRegister.W(), UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Ldr(tempRegister, MemoryOperand(tempRegister, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(tempRegister);
}

void AsmInterpreterCall::PushFrameState(ExtendedAssembler *assembler, Register prevSp, Register fp,
    Register currentSlot, Register callTarget, Register thisObj, Register method, Register pc, Register op)
{
    __ Mov(op, Immediate(static_cast<int32_t>(FrameType::ASM_INTERPRETER_FRAME)));
    __ Stp(prevSp, op, MemoryOperand(currentSlot, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX)); // -2: frame type & prevSp
    __ Ldr(pc, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    __ Stp(fp, pc, MemoryOperand(currentSlot, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX)); // -2: pc & fp
    __ Ldr(op, MemoryOperand(callTarget, JSFunction::LEXICAL_ENV_OFFSET));
    __ Stp(op, Register(Zero), MemoryOperand(currentSlot,
                                             -2 * FRAME_SLOT_SIZE, // -2: jumpSizeAfterCall & env
                                             AddrMode::PREINDEX));
    __ Mov(op, Immediate(JSTaggedValue::VALUE_HOLE));
    __ Stp(thisObj, op, MemoryOperand(currentSlot, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));    // -2: acc & this
    __ Str(callTarget, MemoryOperand(currentSlot, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));         // -1: callTarget
}

void AsmInterpreterCall::GetNumVregsFromCallField(ExtendedAssembler *assembler, Register callField, Register numVregs)
{
    __ Mov(numVregs, callField);
    __ Lsr(numVregs, numVregs, MethodLiteral::NumVregsBits::START_BIT);
    __ And(numVregs.W(), numVregs.W(), LogicalImmediate::Create(
        MethodLiteral::NumVregsBits::Mask() >> MethodLiteral::NumVregsBits::START_BIT, RegWSize));
}

void AsmInterpreterCall::GetDeclaredNumArgsFromCallField(ExtendedAssembler *assembler, Register callField,
    Register declaredNumArgs)
{
    __ Mov(declaredNumArgs, callField);
    __ Lsr(declaredNumArgs, declaredNumArgs, MethodLiteral::NumArgsBits::START_BIT);
    __ And(declaredNumArgs.W(), declaredNumArgs.W(), LogicalImmediate::Create(
        MethodLiteral::NumArgsBits::Mask() >> MethodLiteral::NumArgsBits::START_BIT, RegWSize));
}

void AsmInterpreterCall::PushAsmInterpEntryFrame(ExtendedAssembler *assembler)
{
    Register glue = __ GlueRegister();
    Register fp(X29);
    Register sp(SP);

    size_t begin = __ GetCurrentPosition();
    if (!assembler->FromInterpreterHandler()) {
        __ CalleeSave();
    }

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register prevFrameRegister = __ TempRegister1();
    [[maybe_unused]] TempRegister2Scope scope2(assembler);
    Register frameTypeRegister = __ TempRegister2();

    __ PushFpAndLr();

    // prev managed fp is leave frame or nullptr(the first frame)
    __ Ldr(prevFrameRegister, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Mov(frameTypeRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_ENTRY_FRAME)));
    // 2 : prevSp & frame type
    __ Stp(prevFrameRegister, frameTypeRegister, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2 : pc & glue
    __ Stp(glue, Register(Zero), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // pc
    if (!assembler->FromInterpreterHandler()) {
        size_t end = __ GetCurrentPosition();
        if ((end - begin) != FrameCompletionPos::ARM64CppToAsmInterp) {
            LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::ARM64CppToAsmInterp
                                << "This frame has been modified, and the offset CppToAsmInterp should be updated too.";
        }
    }
    __ Add(fp, sp, Immediate(4 * FRAME_SLOT_SIZE));  // 32: skip frame type, prevSp, pc and glue
}

void AsmInterpreterCall::PopAsmInterpEntryFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register prevFrameRegister = __ TempRegister1();
    [[maybe_unused]] TempRegister2Scope scope2(assembler);
    Register glue = __ TempRegister2();
    // 2: glue & pc
    __ Ldp(glue, Register(Zero), MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    // 2: skip frame type & prev
    __ Ldp(prevFrameRegister, Register(Zero), MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Str(prevFrameRegister, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    size_t begin = __ GetCurrentPosition();
    __ RestoreFpAndLr();
    if (!assembler->FromInterpreterHandler()) {
        __ CalleeRestore();
        size_t end = __ GetCurrentPosition();
        if ((end - begin) != FrameCompletionPos::ARM64AsmInterpToCpp) {
            LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::ARM64AsmInterpToCpp
                                << "This frame has been modified, and the offset AsmInterpToCpp should be updated too.";
        }
    }
}

void AsmInterpreterCall::PushGeneratorFrameState(ExtendedAssembler *assembler, Register &prevSpRegister,
    Register &fpRegister, Register &currentSlotRegister, Register &callTargetRegister, Register &thisRegister,
    Register &methodRegister, Register &contextRegister, Register &pcRegister, Register &operatorRegister)
{
    __ Mov(operatorRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_FRAME)));
    __ Stp(prevSpRegister, operatorRegister,
        MemoryOperand(currentSlotRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // 2 : frameType and prevSp
    __ Ldr(pcRegister, MemoryOperand(methodRegister, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    // offset need 8 align, GENERATOR_NREGS_OFFSET instead of GENERATOR_BC_OFFSET_OFFSET
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_NREGS_OFFSET));
    // 32: get high 32bit
    __ Lsr(operatorRegister, operatorRegister, 32);
    __ Add(pcRegister, operatorRegister, pcRegister);
    // 2 : pc and fp
    __ Stp(fpRegister, pcRegister, MemoryOperand(currentSlotRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // jumpSizeAfterCall
    __ Str(Register(Zero), MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_LEXICALENV_OFFSET));
    // env
    __ Str(operatorRegister, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_ACC_OFFSET));
    // acc
    __ Str(operatorRegister, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Stp(callTargetRegister, thisRegister,
        MemoryOperand(currentSlotRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // 2 : acc and callTarget
}

void AsmInterpreterCall::CallBCStub(ExtendedAssembler *assembler, Register &newSp, Register &glue,
    Register &method, Register &pc, Register &temp)
{
    // prepare call entry
    __ Mov(Register(X19), glue);    // X19 - glue
    __ Mov(Register(FP), newSp);    // FP - sp
    __ Mov(Register(X20), pc);      // X20 - pc
    __ Ldr(Register(X21), MemoryOperand(method, Method::CONSTANT_POOL_OFFSET));   // X21 - constantpool
    __ Ldr(Register(X22), MemoryOperand(method, Method::PROFILE_TYPE_INFO_OFFSET)); // X22 - profileTypeInfo
    __ Mov(Register(X23), Immediate(JSTaggedValue::Hole().GetRawData()));                   // X23 - acc
    __ Ldr(Register(X24), MemoryOperand(method, Method::LITERAL_INFO_OFFSET)); // X24 - hotnessCounter

    // call the first bytecode handler
    __ Ldrb(temp.W(), MemoryOperand(pc, 0));
    // 3 : 3 means *8
    __ Add(temp, glue, Operand(temp.W(), UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Ldr(temp, MemoryOperand(temp, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(temp);
}

void AsmInterpreterCall::CallNativeEntry(ExtendedAssembler *assembler)
{
    Register glue(X0);
    Register argv(X5);
    Register method(X2);
    Register function(X1);
    Register nativeCode(X7);
    Register temp(X9);

    Register sp(SP);
    // 2: function & align
    __ Stp(function, Register(Zero), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2: skip argc & thread
    __ Sub(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_ENTRY_FRAME, temp, argv);
    // get native pointer
    __ Ldr(nativeCode, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    __ Mov(temp, argv);
    __ Sub(Register(X0), temp, Immediate(2 * FRAME_SLOT_SIZE));  // 2: skip argc & thread
    CallNativeInternal(assembler, nativeCode);

    // 4: skip function
    __ Add(sp, sp, Immediate(4 * FRAME_SLOT_SIZE));
    __ Ret();
}

void AsmInterpreterCall::ThrowStackOverflowExceptionAndReturn(ExtendedAssembler *assembler, Register glue,
    Register fp, Register op)
{
    if (fp != Register(SP)) {
        __ Mov(Register(SP), fp);
    }
    __ Mov(op, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException));
    // 3 : 3 means *8
    __ Add(op, glue, Operand(op, LSL, 3));
    __ Ldr(op, MemoryOperand(op, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    if (glue.GetId() != X0) {
        __ Mov(Register(X0), glue);
    }
    __ Blr(op);
    __ RestoreFpAndLr();
    __ Ret();
}
#undef __
}  // panda::ecmascript::aarch64