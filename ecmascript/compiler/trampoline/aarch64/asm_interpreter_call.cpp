/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/compiler/trampoline/aarch64/common_call.h"

#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
#define __ assembler->

// Generate code for entering asm interpreter
// c++ calling convention
// Input: glue           - %x0
//        callTarget     - %x1
//        method         - %x2
//        callField      - %x3
//        argc           - %x4
//        argv           - %x5(<callTarget, newTarget, this> are at the beginning of argv)
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

// Input: glue           - %x0
//        callTarget     - %x1
//        method         - %x2
//        callField      - %x3
//        argc           - %x4
//        argv           - %x5(<callTarget, newTarget, this> are at the beginning of argv)
void AsmInterpreterCall::AsmInterpEntryDispatch(ExtendedAssembler *assembler)
{
    Label notJSFunction;
    Label callNativeEntry;
    Label callJSFunctionEntry;
    Label notCallable;
    Register glueRegister = x0;
    Register argcRegister = w4;
    Register argvRegister = x5;
    Register callTargetRegister = x1;
    Register callFieldRegister = x3;
    Register bitFieldRegister = x16;
    Register tempRegister = x17; // can not be used to store any variable
    Register functionTypeRegister = w18;
    __ Ldr(tempRegister, MemoryOperand(callTargetRegister, TaggedObject::HCLASS_OFFSET));
    __ And(tempRegister, tempRegister, LogicalImmediate::Create(TaggedObject::GC_STATE_MASK, X_REG_SIZE));
    __ Ldr(bitFieldRegister, MemoryOperand(tempRegister, JSHClass::BIT_FIELD_OFFSET));
    __ And(functionTypeRegister, bitFieldRegister.W(), LogicalImmediate::Create(0xFF, W_REG_SIZE));
    __ Mov(tempRegister.W(), Immediate(static_cast<int64_t>(JSType::JS_FUNCTION_FIRST)));
    __ Cmp(functionTypeRegister, tempRegister.W());
    __ B(Condition::LO, &notJSFunction);
    __ Mov(tempRegister.W(), Immediate(static_cast<int64_t>(JSType::JS_FUNCTION_LAST)));
    __ Cmp(functionTypeRegister, tempRegister.W());
    __ B(Condition::LS, &callJSFunctionEntry);
    __ Bind(&notJSFunction);
    {
        __ Tst(bitFieldRegister,
            LogicalImmediate::Create(static_cast<int64_t>(1ULL << JSHClass::CallableBit::START_BIT), X_REG_SIZE));
        __ B(Condition::EQ, &notCallable);
        CallNativeEntry(assembler, false);
    }
    __ Bind(&callNativeEntry);
    CallNativeEntry(assembler, true);
    __ Bind(&callJSFunctionEntry);
    {
        __ Tbnz(callFieldRegister, Method::IsNativeBit::START_BIT, &callNativeEntry);
        // fast path
        __ Add(argvRegister, argvRegister, Immediate(NUM_MANDATORY_JSFUNC_ARGS * JSTaggedValue::TaggedTypeSize()));
        JSCallCommonEntry(assembler, JSCallMode::CALL_ENTRY, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
    }
    __ Bind(&notCallable);
    {
        Register runtimeId = x11;
        Register trampoline = x12;
        __ Mov(runtimeId, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowNotCallableException));
        // 3 : 3 means *8
        __ Add(trampoline, glueRegister, Operand(runtimeId, LSL, 3));
        __ Ldr(trampoline, MemoryOperand(trampoline, JSThread::GlueData::GetRTStubEntriesOffset(false)));
#ifdef ENABLE_CMC_IR_FIX_REGISTER
        __ Mov(x28, glueRegister); // move glue to a callee-save register
#endif
        __ Blr(trampoline);
        __ UpdateGlueAndReadBarrier();
        __ Ret();
    }
}

void AsmInterpreterCall::JSCallCommonEntry(ExtendedAssembler *assembler,
    JSCallMode mode, FrameTransitionType type)
{
    Label stackOverflow;
    Register glueRegister = __ GlueRegister();
    __ UpdateGlueAndReadBarrier(glueRegister);
    Register fpRegister = __ AvailableRegister1();
    Register currentSlotRegister = __ AvailableRegister3();
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
    if (!kungfu::AssemblerModule::IsJumpToCallCommonEntry(mode) || type == FrameTransitionType::BASELINE_TO_OTHER ||
        type == FrameTransitionType::BASELINE_TO_BASELINE_CHECK) {
        __ PushFpAndLr();
    }
    // save fp
    __ Mov(fpRegister, sp);
    __ Mov(currentSlotRegister, sp);

    {
        // Reserve enough sp space to prevent stack parameters from being covered by cpu profiler.
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register tempRegister = __ TempRegister1();
        __ Ldr(tempRegister, MemoryOperand(glueRegister, JSThread::GlueData::GetStackLimitOffset(false)));
        __ Mov(sp, tempRegister);
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
    PushCallThis(assembler, mode, &stackOverflow, type);
    __ Bind(&slowPathEntry);
    JSCallCommonSlowPath(assembler, mode, &fastPathEntry, &pushCallThis, &stackOverflow);

    __ Bind(&stackOverflow);
    if (kungfu::AssemblerModule::IsJumpToCallCommonEntry(mode)) {
        __ Mov(sp, fpRegister);
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register temp = __ TempRegister1();
        // only glue and acc are useful in exception handler
        if (glueRegister != x19) {
            __ Mov(x19, glueRegister);
        }
        Register acc = x23;
        __ Mov(acc, Immediate(JSTaggedValue::VALUE_EXCEPTION));
        Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);
        Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
        // Reload pc to make sure stack trace is right
        __ Mov(temp, callTargetRegister);
        __ Ldr(x20, MemoryOperand(methodRegister, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        // Reload constpool and profileInfo to make sure gc map work normally
        __ Ldr(x22, MemoryOperand(temp, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
        __ Ldr(x22, MemoryOperand(x22, ProfileTypeInfoCell::VALUE_OFFSET));
        __ Ldr(x21, MemoryOperand(methodRegister, Method::CONSTANT_POOL_OFFSET));

        __ Mov(temp, Immediate(kungfu::BytecodeStubCSigns::ID_ThrowStackOverflowException));
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
    } else {
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
        if (stackOverflow != nullptr) {
            [[maybe_unused]] TempRegister1Scope scope(assembler);
            Register op = __ TempRegister1();
            Register numRegister = __ AvailableRegister2();
            __ Mov(numRegister, Immediate(argc));
            StackOverflowCheck(assembler, glueRegister, currentSlotRegister, numRegister, op, stackOverflow);
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
    __ Tbz(callFieldRegister, Method::HaveExtraBit::START_BIT, &noExtraEntry);
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
        __ Tbnz(callFieldRegister, Method::HaveExtraBit::START_BIT, fastPathEntry);
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
        case JSCallMode::CALL_THIS_ARG2_WITH_RETURN:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
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
    return invalidReg;
}

Register AsmInterpreterCall::GetNewTargetRegsiter(ExtendedAssembler *assembler, JSCallMode mode,
    Register defaultRegister)
{
    switch (mode) {
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG3);
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
    return invalidReg;
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
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_WITH_ARGV, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallRangeAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_WITH_ARGV, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallNewAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallNewAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushSuperCallAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushSuperCallAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::SUPER_CALL_WITH_ARGV, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallArgs3AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs3AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG3, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallArgs2AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs2AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG2, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallArg1AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArg1AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG1, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallArg0AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArg0AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG0, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallThisArg0AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisArg0AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG0, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallThisArg1AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisArg1AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG1, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallThisArgs2AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisArgs2AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG2, FrameTransitionType::OTHER_TO_OTHER);
}

void AsmInterpreterCall::PushCallThisArgs3AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallThisArgs3AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG3, FrameTransitionType::OTHER_TO_OTHER);
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

void AsmInterpreterCall::PushNewTargetAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushNewTargetAndDispatchNative));
    CallNativeWithArgv(assembler, true, true);
}

void AsmInterpreterCall::CallNativeWithArgv(ExtendedAssembler *assembler, bool callNew, bool hasNewTarget)
{
    Register glue = x0;
    Register nativeCode = x1;
    Register callTarget = x2;
    Register thisObj = x3;
    Register argc = x4;
    Register argv = x5;
    Register newTarget = x6;
    Register opArgc = x8;
    Register opArgv = x9;
    Register temp = x10;
    Register currentSlotRegister = x11;

    Label pushThis;
    Label stackOverflow;
    bool isFrameComplete = PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME_WITH_ARGV, temp, argc);

    __ Mov(currentSlotRegister, sp);
    // Reserve enough sp space to prevent stack parameters from being covered by cpu profiler.
    __ Ldr(temp, MemoryOperand(glue, JSThread::GlueData::GetStackLimitOffset(false)));
    __ Mov(sp, temp);

    __ Mov(opArgc, argc);
    __ Mov(opArgv, argv);
    PushArgsWithArgv(assembler, glue, opArgc, opArgv, temp, currentSlotRegister, &pushThis, &stackOverflow);

    __ Bind(&pushThis);
    // newTarget
    if (callNew) {
        if (hasNewTarget) {
            // 16: this & newTarget
            __ Stp(newTarget, thisObj, MemoryOperand(currentSlotRegister, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
        } else {
            // 16: this & newTarget
            __ Stp(callTarget, thisObj, MemoryOperand(currentSlotRegister, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
        }
    } else {
        __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        // 16: this & newTarget
        __ Stp(temp, thisObj, MemoryOperand(currentSlotRegister, -DOUBLE_SLOT_SIZE, AddrMode::PREINDEX));
    }
    // callTarget
    __ Str(callTarget, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(temp, currentSlotRegister, Immediate(QUINTUPLE_SLOT_SIZE));
    if (!isFrameComplete) {
        __ Add(fp, temp, Operand(argc, LSL, 3));  // 3: argc * 8
    }

    __ Add(temp, argc, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    // 2: thread & argc
    __ Stp(glue, temp, MemoryOperand(currentSlotRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
#ifdef ENABLE_CMC_IR_FIX_REGISTER
    Register calleeSaveGlue = x28;
    __ Mov(calleeSaveGlue, glue); // move glue to a callee-save register
#endif
    __ Add(x0, currentSlotRegister, Immediate(0));

    __ Align16(currentSlotRegister);
    __ Mov(sp, currentSlotRegister);

    CallNativeInternal(assembler, nativeCode);
    __ Ret();

    __ Bind(&stackOverflow);
    {
        // use builtin_with_argv_frame to mark gc map
        Register frameType = x11;
        __ Ldr(temp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
        __ Mov(sp, temp);
        __ Mov(frameType, Immediate(static_cast<int32_t>(FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME)));
        // 2: frame type and argc
        __ Stp(xzr, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        // 2: fill this&newtgt slots
        __ Stp(temp, temp, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        // 2: fill func&align slots
        __ Stp(xzr, temp, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ Mov(temp, sp);
        // 6：frame type, argc, this, newTarget, func and align
        // +----------------------------------------------------------------+ <---- fp = sp + 6 * frame_slot_size
        // |     FrameType =  BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME |
        // +----------------------------------------------------------------+
        // |                           argc = 0                             |
        // |----------------------------------------------------------------|
        // |                       this = undefined                         |
        // |----------------------------------------------------------------|
        // |                      newTarget = undefine                      |
        // |----------------------------------------------------------------|
        // |                       function = undefined                     |
        // |----------------------------------------------------------------|
        // |                               align                            |
        // +----------------------------------------------------------------+  <---- sp
        __ Add(fp, temp, Immediate(FRAME_SLOT_SIZE * 6));

        Register runtimeId = x11;
        Register trampoline = x12;
        __ Mov(runtimeId, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException));
        // 3 : 3 means *8
        __ Add(trampoline, glue, Operand(runtimeId, LSL, 3));
        __ Ldr(trampoline, MemoryOperand(trampoline, JSThread::GlueData::GetRTStubEntriesOffset(false)));
#ifdef ENABLE_CMC_IR_FIX_REGISTER
        __ Mov(x28, glue); // move glue to a callee-save register
#endif
        __ Blr(trampoline);
        __ UpdateGlueAndReadBarrier();

        // resume rsp
        __ Mov(sp, fp);
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

    Register nativeCode = x0;
    Register glue = x1;
    Register argv = x5;
    Register temp = x6;
    Register nativeCodeTemp = x2;

    __ Mov(nativeCodeTemp, nativeCode);

    __ Ldr(glue, MemoryOperand(sp, 0));
    __ Add(x0, sp, Immediate(0));
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME, temp, argv);

#ifdef ENABLE_CMC_IR_FIX_REGISTER
    Register calleeSaveGlue = x28;
    __ Mov(calleeSaveGlue, glue); // move glue to a callee-save register
#endif
    CallNativeInternal(assembler, nativeCodeTemp);
    __ Ret();
}

bool AsmInterpreterCall::PushBuiltinFrame(ExtendedAssembler *assembler, Register glue,
    FrameType type, Register op, Register next)
{
    __ PushFpAndLr();
    __ Mov(op, sp);
    __ Str(op, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Mov(op, Immediate(static_cast<int32_t>(type)));
    if (type == FrameType::BUILTIN_FRAME) {
        // push stack args
        __ Add(next, sp, Immediate(BuiltinFrame::GetStackArgsToFpDelta(false)));
        // 2: -2 * FRAME_SLOT_SIZE means type & next
        __ Stp(next, op, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        // 2: 2 * FRAME_SLOT_SIZE means skip next and frame type
        __ Add(fp, sp, Immediate(2 * FRAME_SLOT_SIZE));
        return true;
    } else if (type == FrameType::BUILTIN_ENTRY_FRAME) {
        // 2: -2 * FRAME_SLOT_SIZE means type & next
        __ Stp(next, op, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        // 2: 2 * FRAME_SLOT_SIZE means skip next and frame type
        __ Add(fp, sp, Immediate(2 * FRAME_SLOT_SIZE));
        return true;
    } else if (type == FrameType::BUILTIN_FRAME_WITH_ARGV) {
        // this frame push stack args must before update FP, otherwise cpu profiler maybe visit incomplete stack
        // BuiltinWithArgvFrame layout please see frames.h
        // 2: -2 * FRAME_SLOT_SIZE means type & next
        __ Stp(next, op, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        return false;
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void AsmInterpreterCall::CallNativeInternal(ExtendedAssembler *assembler, Register nativeCode)
{
    __ Blr(nativeCode);
    __ UpdateGlueAndReadBarrier();
    // resume rsp
    __ Mov(sp, fp);
    __ RestoreFpAndLr();
}

// ResumeRspAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// x19 - glue
// fp  - sp
// x20 - pc
// x21 - constantPool
// x22 - profileTypeInfo
// x23 - acc
// x24 - hotnessCounter
// x25 - jumpSizeAfterCall
void AsmInterpreterCall::ResumeRspAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndDispatch));

    Register glueRegister = __ GlueRegister();
    Register rsp = sp;
    Register currentSp = fp;
    Register pc = x20;
    Register jumpSizeRegister = x25;

    Register ret = x23;
    Register opcode = w6;
    Register temp = x7;
    Register bcStub = x7;
    Register fpReg = x8;

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
    __ Ldur(fpReg, MemoryOperand(currentSp, fpOffset));  // store fp for temporary
    __ Cmp(jumpSizeRegister, Immediate(0));
    __ B(Condition::LE, &newObjectRangeReturn);
    __ Ldur(currentSp, MemoryOperand(currentSp, spOffset));  // update currentSp

    __ Add(pc, pc, Operand(jumpSizeRegister, LSL, 0));
    __ Ldrb(opcode, MemoryOperand(pc, 0));
    __ Bind(&dispatch);
    {
        __ Mov(rsp, fpReg);  // resume rsp
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
        __ Ldur(ret, MemoryOperand(currentSp, thisOffset));  // update acc
        __ Ldur(currentSp, MemoryOperand(currentSp, spOffset));  // update currentSp
        __ Mov(rsp, fpReg);  // resume rsp
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
        __ And(temp, temp, LogicalImmediate::Create(TaggedObject::GC_STATE_MASK, X_REG_SIZE));
        __ Ldr(temp, MemoryOperand(temp, JSHClass::BIT_FIELD_OFFSET));
        __ And(temp.W(), temp.W(), LogicalImmediate::Create(0xFF, W_REG_SIZE));
        __ Cmp(temp.W(), Immediate(static_cast<int64_t>(JSType::ECMA_OBJECT_LAST)));
        __ B(Condition::HI, &notEcmaObject);
        __ Cmp(temp.W(), Immediate(static_cast<int64_t>(JSType::ECMA_OBJECT_FIRST)));
        __ B(Condition::LO, &notEcmaObject);
        // acc is ecma object
        __ Ldur(currentSp, MemoryOperand(currentSp, spOffset));  // update currentSp
        __ Sub(pc, pc, jumpSizeRegister); // sub negative jmupSize
        __ Ldrb(opcode, MemoryOperand(pc, 0));
        __ B(&dispatch);

        __ Bind(&notEcmaObject);
        {
            int64_t constructorOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFunctionOffset(false))
                - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
            ASSERT(constructorOffset < 0);
            __ Ldur(temp, MemoryOperand(currentSp, constructorOffset));  // load constructor
            __ Ldr(temp, MemoryOperand(temp, JSFunctionBase::METHOD_OFFSET));
            __ Ldr(temp, MemoryOperand(temp, Method::EXTRA_LITERAL_INFO_OFFSET));
            __ Lsr(temp.W(), temp.W(), Method::FunctionKindBits::START_BIT);
            __ And(temp.W(), temp.W(),
                LogicalImmediate::Create((1LU << Method::FunctionKindBits::SIZE) - 1, W_REG_SIZE));
            __ Cmp(temp.W(), Immediate(static_cast<int64_t>(FunctionKind::CLASS_CONSTRUCTOR)));
            __ B(Condition::LS, &getThis);  // constructor is base
            // exception branch
            {
                __ Mov(opcode, Immediate(kungfu::BytecodeStubCSigns::ID_NewObjectRangeThrowException));
                __ Ldur(currentSp, MemoryOperand(currentSp, spOffset));  // update sp
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
    Register rsp = sp;
    Register currentSp = x20;

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
        __ Mov(x0, x19);
        __ Ret();
    }
}

// ResumeRspAndReturnBaseline(uintptr_t acc)
// GHC calling convention
// X19 - glue
// FP - acc
// X20 - prevSp
// X21 - sp
// X22 - jumpSizeAfterCall
void AsmInterpreterCall::ResumeRspAndReturnBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndReturnBaseline));
    Register glue = x19;
    Register rsp = sp;
    Register currentSp = x21;

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register fpRegister = __ TempRegister1();
    int64_t fpOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false)) -
        static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(fpOffset < 0);
    __ Ldur(fpRegister, MemoryOperand(currentSp, fpOffset));
    __ Mov(rsp, fpRegister);
    __ RestoreFpAndLr();
    __ Mov(x0, fp);

    // Check and set result
    Register ret = x0;
    Register jumpSizeRegister = x22;
    Label getThis;
    Label notUndefined;
    Label normalReturn;
    Label newObjectRangeReturn;
    __ Cmp(jumpSizeRegister, Immediate(0));
    __ B(Condition::GT, &normalReturn);

    __ Bind(&newObjectRangeReturn);
    {
        __ Cmp(ret, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(Condition::NE, &notUndefined);

        __ Bind(&getThis);
        int64_t thisOffset = static_cast<int64_t>(AsmInterpretedFrame::GetThisOffset(false)) -
            static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
        ASSERT(thisOffset < 0);
        __ Ldur(ret, MemoryOperand(currentSp, thisOffset));  // update result
        __ B(&normalReturn);

        __ Bind(&notUndefined);
        {
            Register temp = x19;
            Label notEcmaObject;
            __ Mov(temp, Immediate(JSTaggedValue::TAG_HEAPOBJECT_MASK));
            __ And(temp, temp, ret);
            __ Cmp(temp, Immediate(0));
            __ B(Condition::NE, &notEcmaObject);
            // acc is heap object
            __ Ldr(temp, MemoryOperand(ret, TaggedObject::HCLASS_OFFSET));
            __ And(temp, temp, LogicalImmediate::Create(TaggedObject::GC_STATE_MASK, X_REG_SIZE));
            __ Ldr(temp, MemoryOperand(temp, JSHClass::BIT_FIELD_OFFSET));
            __ And(temp.W(), temp.W(), LogicalImmediate::Create(0xFF, W_REG_SIZE));
            __ Cmp(temp.W(), Immediate(static_cast<int64_t>(JSType::ECMA_OBJECT_LAST)));
            __ B(Condition::HI, &notEcmaObject);
            __ Cmp(temp.W(), Immediate(static_cast<int64_t>(JSType::ECMA_OBJECT_FIRST)));
            __ B(Condition::LO, &notEcmaObject);
            // acc is ecma object
            __ B(&normalReturn);

            __ Bind(&notEcmaObject);
            {
                int64_t funcOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFunctionOffset(false)) -
                    static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
                ASSERT(funcOffset < 0);
                __ Ldur(temp, MemoryOperand(currentSp, funcOffset));  // load constructor
                __ Ldr(temp, MemoryOperand(temp, JSFunctionBase::METHOD_OFFSET));
                __ Ldr(temp, MemoryOperand(temp, Method::EXTRA_LITERAL_INFO_OFFSET));
                __ Lsr(temp.W(), temp.W(), Method::FunctionKindBits::START_BIT);
                __ And(temp.W(), temp.W(),
                       LogicalImmediate::Create((1LU << Method::FunctionKindBits::SIZE) - 1, W_REG_SIZE));
                __ Cmp(temp.W(), Immediate(static_cast<int64_t>(FunctionKind::CLASS_CONSTRUCTOR)));
                __ B(Condition::LS, &getThis);  // constructor is base
                // fall through
            }
        }
    }
    __ Bind(&normalReturn);
    __ Ret();
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

    Register glue = x19;
    Register pc = x20;
    Register fpReg = x5;
    Register opcode = w6;
    Register bcStub = x7;

    Label dispatch;
    __ Ldr(fpReg, MemoryOperand(glue, JSThread::GlueData::GetLastFpOffset(false)));
    __ Cmp(fpReg, Immediate(0));
    __ B(Condition::EQ, &dispatch);
    // up frame
    __ Mov(sp, fpReg);
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

    Register glue = x19;
    Register fpReg = x5;
    Register acc = x20;
    Register cppRet = x0;

    __ Ldr(fpReg, MemoryOperand(glue, JSThread::GlueData::GetLastFpOffset(false)));
    __ Mov(sp, fpReg);
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
    Register rsp = sp;
    Register currentSp = fp;
    Register pc = x20;
    Register jumpSizeRegister = x25;

    Register ret = x23;
    Register opcode = w6;
    Register bcStub = x7;
    Register fpReg = x8;

    int64_t fpOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    int64_t spOffset = static_cast<int64_t>(AsmInterpretedFrame::GetBaseOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    int64_t funcOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFunctionOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(fpOffset < 0);
    ASSERT(spOffset < 0);
    ASSERT(funcOffset < 0);

    __ Ldur(fpReg, MemoryOperand(currentSp, fpOffset));  // store fp for temporary
    __ Ldur(ret, MemoryOperand(currentSp, funcOffset)); // restore acc
    __ Ldur(currentSp, MemoryOperand(currentSp, spOffset));  // update currentSp

    __ Add(pc, pc, Operand(jumpSizeRegister, LSL, 0));
    __ Ldrb(opcode, MemoryOperand(pc, 0));

    __ Mov(rsp, fpReg);  // resume rsp
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
        JSCallCommonEntry(assembler, JSCallMode::CALL_GETTER, FrameTransitionType::OTHER_TO_OTHER);
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
        JSCallCommonEntry(assembler, JSCallMode::CALL_SETTER, FrameTransitionType::OTHER_TO_OTHER);
    }
}

void AsmInterpreterCall::CallContainersArgs2(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallContainersArgs2));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG2_WITH_RETURN,
                          FrameTransitionType::OTHER_TO_OTHER);
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
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG3_WITH_RETURN,
                          FrameTransitionType::OTHER_TO_OTHER);
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
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARGV_WITH_RETURN,
                          FrameTransitionType::OTHER_TO_OTHER);
    }
}

// preserve all the general registers, except x15 and callee saved registers/
// and call x15
void AsmInterpreterCall::PreserveMostCall(ExtendedAssembler* assembler)
{
    // * layout as the following:
    //               +--------------------------+ ---------
    //               |       . . . . .          |         ^
    // callerSP ---> |--------------------------|         |
    //               |       returnAddr         |         |
    //               |--------------------------|   OptimizedFrame
    //               |       callsiteFp         |         |
    //       fp ---> |--------------------------|         |
    //               |     OPTIMIZED_FRAME      |         v
    //               +--------------------------+ ---------
    //               |           x0             |
    //               +--------------------------+
    //               |           x1             |
    //               +--------------------------+
    //               |           r2             |
    //               +--------------------------+
    //               |           x3             |
    //               +--------------------------+
    //               |           x4             |
    //               +--------------------------+
    //               |           x5             |
    //               +--------------------------+
    //               |           x6             |
    //               +--------------------------+
    //               |           x7             |
    //               +--------------------------+
    //               |           x8             |
    //               +--------------------------+
    //               |           x9             |
    //               +--------------------------+
    //               |           x10            |
    //               +--------------------------+
    //               |           x11            |
    //               +--------------------------+
    //               |           x12            |
    //               +--------------------------+
    //               |           x13            |
    //               +--------------------------+
    //               |           x14            |
    //               +--------------------------+
    //               |           x16            |
    //               +--------------------------+
    //               |           x17            |
    //               +--------------------------+
    //               |           x18            |
    //               +--------------------------+
    //               |         align            |
    // calleeSP ---> +--------------------------+
    {
        // prologue to save fp, frametype, and update fp.
        __ Stp(fp, lr, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
        // Zero register means OPTIMIZED_FRAME
        __ Stp(x0, xzr, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
        __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));
    }
    int32_t PreserveRegPairIndex = 9;
    // x0~x14,x16,x17,x18 should be preserved,
    // other general registers are callee saved register, callee will save them.
    __ Sub(sp, sp, Immediate(DOUBLE_SLOT_SIZE * PreserveRegPairIndex));
    __ Stp(x1, x2, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(x3, x4, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(x5, x6, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(x7, x8, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(x9, x10, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(x11, x12, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(x13, x14, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(x16, x17, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Str(x18, MemoryOperand(sp, FRAME_SLOT_SIZE));
    __ Blr(x15);
    __ Ldr(x18, MemoryOperand(sp, FRAME_SLOT_SIZE));
    __ Ldp(x16, x17, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(x13, x14, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(x11, x12, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(x9, x10, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(x7, x8, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(x5, x6, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(x3, x4, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(x1, x2, MemoryOperand(sp, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldr(x0, MemoryOperand(sp, DOUBLE_SLOT_SIZE * PreserveRegPairIndex));
    {
        // epilogue to restore sp, fp, lr.
        // Skip x0 slot and frametype slot
        __ Add(sp, sp, Immediate(DOUBLE_SLOT_SIZE * PreserveRegPairIndex +
            FRAME_SLOT_SIZE + FRAME_SLOT_SIZE));
        __ Ldp(fp, lr, MemoryOperand(sp, DOUBLE_SLOT_SIZE, AddrMode::POSTINDEX));
        __ Ret();
    }
}

// ASMFastWriteBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value)
// c calling convention, but preserve all general registers except %x15
// %x0 - glue
// %x1 - obj
// %x2 - offset
// %x3 - value
void AsmInterpreterCall::ASMFastWriteBarrier(ExtendedAssembler* assembler)
{
    // valid region flag are as follows, assume it will be ALWAYS VALID.
    // Judge the region of value with:
    //                          "young"            "sweepable share"  "readonly share"
    // region flag:         0x08, 0x09, [0x0A, 0x11], [0x12, 0x15],     0x16
    // value is share:                                [0x12,            0x16] =>  valueMaybeSweepableShare
    // readonly share:                                                  0x16  =>  return
    // sweepable share:                               [0x12, 0x15]            =>  needShareBarrier
    // value is not share:  0x08, 0x09, [0x0A, 0x11],                         =>  valueNotShare
    // value is young :           0x09                                        =>  needCallNotShare
    // value is not young : 0x08,       [0x0A, 0x11],                         =>  checkMark
    ASSERT(IN_YOUNG_SPACE < SHARED_SPACE_BEGIN && SHARED_SPACE_BEGIN <= SHARED_SWEEPABLE_SPACE_BEGIN &&
           SHARED_SWEEPABLE_SPACE_END < IN_SHARED_READ_ONLY_SPACE && IN_SHARED_READ_ONLY_SPACE == HEAP_SPACE_END);
    __ BindAssemblerStub(RTSTUB_ID(ASMFastWriteBarrier));

    Label needCall;
    Label checkMark;
    Label needCallNotShare;
    Label needShareBarrier;
    Label valueNotShare;
    Label valueMaybeSweepableShare;
    {
        // int8_t *valueRegion = value & (~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK))
        // int8_t valueFlag = *valueRegion
        // if (valueFlag >= SHARED_SWEEPABLE_SPACE_BEGIN){
        //    goto valueMaybeSweepableShare
        // }

        __ And(x15, x3, LogicalImmediate::Create(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK), X_REG_SIZE));
        // X15 is the region address of value.
        __ Ldrb(w15, MemoryOperand(x15, 0));
        // X15 is the flag load from region of value.
        __ Cmp(w15, Immediate(SHARED_SWEEPABLE_SPACE_BEGIN));
        __ B(GE, &valueMaybeSweepableShare);
        // if value may be SweepableShare, goto valueMaybeSweepableShare
    }
    __ Bind(&valueNotShare);
    {
        // valueNotShare:
        // if (valueFlag != IN_YOUNG_SPACE){
        //      goto checkMark
        // }
        // int8_t *objRegion = obj & (~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK))
        // int8_t objFlag = *objRegion
        // if (objFlag != IN_YOUNG_SPACE){
        //    goto needCallNotShare
        // }

        __ Cmp(w15, Immediate(RegionSpaceFlag::IN_YOUNG_SPACE));
        __ B(NE, &checkMark);
        // if value is not in young, goto checkMark

        __ And(x15, x1, LogicalImmediate::Create(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK), X_REG_SIZE));
        // X15 is the region address of obj.
        __ Ldrb(w15, MemoryOperand(x15, 0));
        // X15 is the flag load from region of obj.
        __ Cmp(w15, Immediate(RegionSpaceFlag::IN_YOUNG_SPACE));
        __ B(NE, &needCallNotShare);
        // if obj is not in young, goto needCallNotShare
    }

    __ Bind(&checkMark);
    {
        // checkMark:
        // int8_t GCStateBitField = *(glue+GCStateBitFieldOffset)
        // if (GCStateBitField & JSThread::CONCURRENT_MARKING_BITFIELD_MASK != 0) {
        //    goto needCallNotShare
        // }
        // return

        __ Mov(x15, Immediate(JSThread::GlueData::GetGCStateBitFieldOffset(false)));
        __ Ldrb(w15, MemoryOperand(x0, x15, UXTX));
        __ Tst(w15, LogicalImmediate::Create(JSThread::CONCURRENT_MARKING_BITFIELD_MASK, W_REG_SIZE));
        __ B(NE, &needCallNotShare);
        // if GCState is not READY_TO_MARK, go to needCallNotShare.
        __ Ret();
    }

    __ Bind(&valueMaybeSweepableShare);
    {
        // valueMaybeSweepableShare:
        // if (valueFlag != IN_SHARED_READ_ONLY_SPACE){
        //    goto needShareBarrier
        // }
        // return
        __ Cmp(w15, Immediate(RegionSpaceFlag::IN_SHARED_READ_ONLY_SPACE));
        __ B(NE, &needShareBarrier);
        __ Ret();
    }

    __ Bind(&needCallNotShare);
    {
        int32_t NonSValueBarrier = static_cast<int32_t>(JSThread::GlueData::GetCOStubEntriesOffset(false)) +
            kungfu::CommonStubCSigns::SetNonSValueWithBarrier * FRAME_SLOT_SIZE;
        __ Mov(x15, Immediate(NonSValueBarrier));
    }
    __ Bind(&needCall);
    {
        __ Ldr(x15, MemoryOperand(x0, x15, UXTX));
        PreserveMostCall(assembler);
    }
    __ Bind(&needShareBarrier);
    {
        ASMFastSharedWriteBarrier(assembler, needCall);
    }
}

void AsmInterpreterCall::LoadBarrierCopyBack(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(LoadBarrierCopyBack));
    Label copyBackTable;
    int ldrOffset = -4;
    __ Ldur(w1, MemoryOperand(x30, ldrOffset));
    __ And(w1, w1, LogicalImmediate::Create(0x1F, W_REG_SIZE));
    __ Adr(x2, &copyBackTable);
    // every item in copyback table is 8 bytes.
    int copyBackItemInsnShift = 3;
    __ Add(x2, x2, Operand(w1, UXTW, copyBackItemInsnShift));
    __ Br(x2);

    __ Bind(&copyBackTable);
    {
        auto MovX0ToXregAndBlLr = [&assembler](Register xreg) {
            __ Mov(xreg, x0);
            __ Br(lr);
        };
        MovX0ToXregAndBlLr(x0);
        MovX0ToXregAndBlLr(x1);
        MovX0ToXregAndBlLr(x2);
        MovX0ToXregAndBlLr(x3);
        MovX0ToXregAndBlLr(x4);
        MovX0ToXregAndBlLr(x5);
        MovX0ToXregAndBlLr(x6);
        MovX0ToXregAndBlLr(x7);
        MovX0ToXregAndBlLr(x8);
        MovX0ToXregAndBlLr(x9);
        MovX0ToXregAndBlLr(x10);
        MovX0ToXregAndBlLr(x11);
        MovX0ToXregAndBlLr(x12);
        MovX0ToXregAndBlLr(x13);
        MovX0ToXregAndBlLr(x14);
        MovX0ToXregAndBlLr(x15);
        MovX0ToXregAndBlLr(x16);
        MovX0ToXregAndBlLr(x17);
        MovX0ToXregAndBlLr(x18);
        MovX0ToXregAndBlLr(x19);
        MovX0ToXregAndBlLr(x20);
        MovX0ToXregAndBlLr(x21);
        MovX0ToXregAndBlLr(x22);
        MovX0ToXregAndBlLr(x23);
        MovX0ToXregAndBlLr(x24);
        MovX0ToXregAndBlLr(x25);
        MovX0ToXregAndBlLr(x26);
        MovX0ToXregAndBlLr(x27);
        MovX0ToXregAndBlLr(x28);
        MovX0ToXregAndBlLr(x29);
        MovX0ToXregAndBlLr(x30);
        MovX0ToXregAndBlLr(sp);
    }
}

// %x0 - glue
// %x1 - obj
// %x2 - offset
// %x3 - value
void AsmInterpreterCall::ASMFastSharedWriteBarrier(ExtendedAssembler* assembler, Label& needCall)
{
    Label checkBarrierForSharedValue;
    Label restoreScratchRegister;
    Label callSharedBarrier;
    {
        // int8_t *objRegion = obj & (~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK))
        // int8_t objFlag = *objRegion
        // if (objFlag >= SHARED_SPACE_BEGIN){
        //    // share to share, just check the barrier
        //    goto checkBarrierForSharedValue
        // }
        __ And(x15, x1, LogicalImmediate::Create(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK), X_REG_SIZE));
        __ Ldrb(w15, MemoryOperand(x15, 0));
        // X15 is the flag load from region of obj.
        __ Cmp(w15, Immediate(RegionSpaceFlag::SHARED_SPACE_BEGIN));
        __ B(GE, &checkBarrierForSharedValue);  // if objflag >= SHARED_SPACE_BEGIN  => checkBarrierForSharedValue
    }
    {
        // int8_t *objRegion = obj & (~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK))
        // int8_t *localToShareSet = *(objRegion + LocalToShareSetOffset)
        // if (localToShareSet == 0){
        //    goto callSharedBarrier
        // }
        __ And(x15, x1, LogicalImmediate::Create(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK), X_REG_SIZE));
        __ Ldr(x15, MemoryOperand(x15, Region::PackedData::GetLocalToShareSetOffset(false)));
        // X15 is localToShareSet for obj region.
        __ Cbz(x15, &callSharedBarrier);   // if localToShareSet == 0  => callSharedBarrier
    }
    {
        // X16, X17 will be used as scratch register, spill them.
        // the caller will call this function with inline asm, it will not save any registers except x15.
        // So we need spill and restore x16, x17 when we need them as scratch register.
        {
            __ Stp(x16, x17, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
        }
        // int64_t objOffset = obj & DEFAULT_REGION_MASK
        // int64_t slotOffset = objOffset + offset
        __ And(x16, x1, LogicalImmediate::Create(DEFAULT_REGION_MASK, X_REG_SIZE));
        __ Add(x16, x16, Operand(x2));

        // the logic to get mask in stub_builder.cpp
        //               [63-------------------------35][34------------------------8][7---3][2-0]
        // bitOffset:                                       bbbbbbbbbbbbbbbbbbbbbbbb  bbbcc  ccc
        // bitPerWordMask:                                                               11  111
        // indexInWord = And bitoffset bitPerWordMask
        // indexInWord:                                                                  cc  ccc
        // mask = 1 << indexInWord

        // the logic to test bit set value here:
        //               [63-------------------------35][34------------------------8][7---3][2-0]
        // slotOffset:    aaaaaaaaaaaaaaaaaaaaaaaaaaaaa  bbbbbbbbbbbbbbbbbbbbbbbbbbb  ccccc  ddd
        // Ubfm X16 slotOffset 3 7
        // indexInWord:                                                                  cc  ccc
        __ Ubfm(x17, x16, TAGGED_TYPE_SIZE_LOG, TAGGED_TYPE_SIZE_LOG + GCBitset::BIT_PER_WORD_LOG2 - 1);

        // the logic to get byteIndex in stub_builder.cpp
        //               [63-------------------------35][34------------------------8][7---3][2-0]
        // slotOffset:    aaaaaaaaaaaaaaaaaaaaaaaaaaaaa  bbbbbbbbbbbbbbbbbbbbbbbbbbb  ccccc  ddd
        // 1. bitOffsetPtr = LSR TAGGED_TYPE_SIZE_LOG(3) slotOffset
        // bitOffsetPtr:     aaaaaaaaaaaaaaaaaaaaaaaaaa  aaabbbbbbbbbbbbbbbbbbbbbbbb  bbbcc  ccc
        // 2. bitOffset = TruncPtrToInt32 bitOffsetPtr
        // bitOffset:                                       bbbbbbbbbbbbbbbbbbbbbbbb  bbbcc  ccc
        // 3. index = LSR BIT_PER_WORD_LOG2(5) bitOffset
        // index:                                                bbbbbbbbbbbbbbbbbbb  bbbbb  bbb
        // 4. byteIndex = Mul index BYTE_PER_WORD(4)
        // byteIndex:                                          bbbbbbbbbbbbbbbbbbbbb  bbbbb  b00

        // the logic to get byteIndex here:
        //               [63-------------------------35][34------------------------8][7---3][2-0]
        // slotOffset:    aaaaaaaaaaaaaaaaaaaaaaaaaaaaa  bbbbbbbbbbbbbbbbbbbbbbbbbbb  ccccc  ddd
        // Ubfm X16 slotOffset 8 34
        // index:                                                bbbbbbbbbbbbbbbbbbb  bbbbb  bbb
        __ Ubfm(x16, x16, TAGGED_TYPE_SIZE_LOG + GCBitset::BIT_PER_WORD_LOG2,
                sizeof(uint32_t) * GCBitset::BIT_PER_BYTE + TAGGED_TYPE_SIZE_LOG - 1);
        __ Add(x15, x15, Operand(x16, LSL, GCBitset::BYTE_PER_WORD_LOG2));
        __ Add(x15, x15, Immediate(RememberedSet::GCBITSET_DATA_OFFSET));
        // X15 is the address of bitset value. X15 = X15 + X16 << BYTE_PER_WORD_LOG2 + GCBITSET_DATA_OFFSET

        // mask = 1 << indexInWord
        __ Mov(w16, Immediate(1));
        __ Lsl(w17, w16, w17); // X17 is the mask

        __ Ldr(w16, MemoryOperand(x15, 0)); // x16: oldsetValue
        __ Tst(w16, w17);
        __ B(NE, &restoreScratchRegister);
        __ Orr(w16, w16, w17);
        __ Str(w16, MemoryOperand(x15, 0));
    }
    __ Bind(&restoreScratchRegister);
    {
        __ Ldp(x16, x17, MemoryOperand(sp, DOUBLE_SLOT_SIZE, POSTINDEX));
    }
    __ Bind(&checkBarrierForSharedValue);
    {
        // checkBarrierForSharedValue:
        // int8_t GCStateBitField = *(glue+SharedGCStateBitFieldOffset)
        // if (GCStateBitField & JSThread::SHARED_CONCURRENT_MARKING_BITFIELD_MASK != 0) {
        //    goto callSharedBarrier
        // }
        // return
        __ Mov(x15, Immediate(JSThread::GlueData::GetSharedGCStateBitFieldOffset(false)));
        __ Ldrb(w15, MemoryOperand(x0, x15, UXTX));
        static_assert(JSThread::SHARED_CONCURRENT_MARKING_BITFIELD_MASK == 1 && "Tbnz can't handle other bit mask");
        __ Tbnz(w15, 0, &callSharedBarrier);
        // if GCState is not READY_TO_MARK, go to needCallNotShare.
        __ Ret();
    }

    __ Bind(&callSharedBarrier);
    {
        int32_t SValueBarrierOffset = static_cast<int32_t>(JSThread::GlueData::GetCOStubEntriesOffset(false)) +
            kungfu::CommonStubCSigns::SetSValueWithBarrier * FRAME_SLOT_SIZE;
        __ Mov(x15, Immediate(SValueBarrierOffset));
        __ B(&needCall);
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
    Register contextRegister = x1;
    Register pc = x8;
    Register prevSpRegister = fp;
    Register callTarget = x4;
    Register method = x5;
    Register temp = x6; // can not be used to store any variable
    Register currentSlotRegister = x7;
    Register fpRegister = x9;
    Register thisRegister = x25;
    Register nRegsRegister = w26;
    Register regsArrayRegister = x27;
    Register newSp = x28;
    __ Ldr(callTarget, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_METHOD_OFFSET));
    __ Ldr(method, MemoryOperand(callTarget, JSFunctionBase::METHOD_OFFSET));
    __ PushFpAndLr();
    // save fp
    __ Mov(fpRegister, sp);
    __ Mov(currentSlotRegister, sp);
    // Reserve enough sp space to prevent stack parameters from being covered by cpu profiler.
    __ Ldr(temp, MemoryOperand(glue, JSThread::GlueData::GetStackLimitOffset(false)));
    __ Mov(sp, temp);
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
    __ Mov(sp, currentSlotRegister);
    // call bc stub
    CallBCStub(assembler, newSp, glue, callTarget, method, pc, temp);

    __ Bind(&stackOverflow);
    {
        ThrowStackOverflowExceptionAndReturn(assembler, glue, fpRegister, temp);
    }
}

void AsmInterpreterCall::PushCallThis(ExtendedAssembler *assembler,
    JSCallMode mode, Label *stackOverflow, FrameTransitionType type)
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
    __ Tst(callFieldRegister, LogicalImmediate::Create(CALL_TYPE_MASK, X_REG_SIZE));
    __ B(Condition::EQ, &pushVregs);
    __ Tbz(callFieldRegister, Method::HaveThisBit::START_BIT, &pushNewTarget);
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
        __ Tbz(callFieldRegister, Method::HaveNewTargetBit::START_BIT, &pushCallTarget);
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
        __ Tbz(callFieldRegister, Method::HaveFuncBit::START_BIT, &pushVregs);
        __ Str(callTargetRegister, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ Bind(&pushVregs);
    {
        PushVregs(assembler, stackOverflow, type);
    }
}

void AsmInterpreterCall::PushVregs(ExtendedAssembler *assembler,
    Label *stackOverflow, FrameTransitionType type)
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
        __ Mov(sp, currentSlotRegister);
        if (type == FrameTransitionType::OTHER_TO_BASELINE_CHECK ||
            type == FrameTransitionType::BASELINE_TO_BASELINE_CHECK) {
            // check baselinecode, temp modify TOOD: need to check
            Label baselineCodeUndefined;
            __ Ldr(tempRegister, MemoryOperand(callTargetRegister, JSFunction::BASELINECODE_OFFSET));
            __ Cmp(tempRegister, Immediate(JSTaggedValue::VALUE_UNDEFINED));
            __ B(Condition::EQ, &baselineCodeUndefined);

            // check is compiling
            __ Cmp(tempRegister, Immediate(JSTaggedValue::VALUE_HOLE));
            __ B(Condition::EQ, &baselineCodeUndefined);

            if (MachineCode::FUNCADDR_OFFSET % 8 == 0) { // 8: imm in 64-bit ldr insn must be a multiple of 8
                __ Ldr(tempRegister, MemoryOperand(tempRegister, MachineCode::FUNCADDR_OFFSET));
            } else {
                ASSERT(MachineCode::FUNCADDR_OFFSET < 256); // 256: imm in ldur insn must be in the range -256 to 255
                __ Ldur(tempRegister, MemoryOperand(tempRegister, MachineCode::FUNCADDR_OFFSET));
            }
            if (glue != x19) {
                __ Mov(x19, glue);
            }
            if (methodRegister != x21) {
                __ Mov(x21, methodRegister);
            }
            __ Mov(currentSlotRegister, Immediate(BASELINEJIT_PC_FLAG));
            // -3: frame type, prevSp, pc
            __ Stur(currentSlotRegister, MemoryOperand(newSpRegister, -3 * FRAME_SLOT_SIZE));
            __ Mov(x29, newSpRegister);
            __ Br(tempRegister);
            __ Bind(&baselineCodeUndefined);
        }
        DispatchCall(assembler, pcRegister, newSpRegister);
    }
}

// Input: X19 - glue
//        FP - sp
//        X20 - callTarget
//        X21 - method
void AsmInterpreterCall::DispatchCall(ExtendedAssembler *assembler, Register pcRegister,
    Register newSpRegister, Register accRegister, bool hasException)
{
    Register glueRegister = __ GlueRegister();
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);

    if (glueRegister != x19) {
        __ Mov(x19, glueRegister);
    }
    __ Ldrh(w24, MemoryOperand(methodRegister, Method::LITERAL_INFO_OFFSET));
    if (!accRegister.IsValid()) {
        __ Mov(x23, Immediate(JSTaggedValue::VALUE_HOLE));
    } else {
        ASSERT(accRegister == x23);
    }
    __ Ldr(x22, MemoryOperand(callTargetRegister, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    __ Ldr(x22, MemoryOperand(x22, ProfileTypeInfoCell::VALUE_OFFSET));
    __ Ldr(x21, MemoryOperand(methodRegister, Method::CONSTANT_POOL_OFFSET));
    __ Mov(x20, pcRegister);
    __ Mov(fp, newSpRegister);

    Register bcIndexRegister = __ AvailableRegister1();
    Register tempRegister = __ AvailableRegister2();
    if (hasException) {
        __ Mov(bcIndexRegister.W(), Immediate(kungfu::BytecodeStubCSigns::ID_ExceptionHandler));
    } else {
        __ Ldrb(bcIndexRegister.W(), MemoryOperand(pcRegister, 0));
    }
    __ Add(tempRegister, glueRegister, Operand(bcIndexRegister.W(), UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Ldr(tempRegister, MemoryOperand(tempRegister, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ UpdateGlueAndReadBarrier(glueRegister);
    __ Br(tempRegister);
}

void AsmInterpreterCall::PushFrameState(ExtendedAssembler *assembler, Register prevSp, Register fpReg,
    Register currentSlot, Register callTarget, Register thisObj, Register method, Register pc, Register op)
{
    __ Mov(op, Immediate(static_cast<int32_t>(FrameType::ASM_INTERPRETER_FRAME)));
    __ Stp(prevSp, op, MemoryOperand(currentSlot, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX)); // -2: frame type & prevSp
    __ Ldr(pc, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    __ Stp(fpReg, pc, MemoryOperand(currentSlot, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX)); // -2: pc & fp
    __ Ldr(op, MemoryOperand(callTarget, JSFunction::LEXICAL_ENV_OFFSET));
    __ Stp(op, xzr, MemoryOperand(currentSlot,
                                  -2 * FRAME_SLOT_SIZE, // -2: jumpSizeAfterCall & env
                                  AddrMode::PREINDEX));
    __ Mov(op, Immediate(JSTaggedValue::VALUE_HOLE));
    __ Stp(thisObj, op, MemoryOperand(currentSlot, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));    // -2: acc & this
    __ Str(callTarget, MemoryOperand(currentSlot, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));         // -1: callTarget
}

void AsmInterpreterCall::GetNumVregsFromCallField(ExtendedAssembler *assembler, Register callField, Register numVregs)
{
    __ Mov(numVregs, callField);
    __ Lsr(numVregs, numVregs, Method::NumVregsBits::START_BIT);
    __ And(numVregs.W(), numVregs.W(), LogicalImmediate::Create(
        Method::NumVregsBits::Mask() >> Method::NumVregsBits::START_BIT, W_REG_SIZE));
}

void AsmInterpreterCall::GetDeclaredNumArgsFromCallField(ExtendedAssembler *assembler, Register callField,
    Register declaredNumArgs)
{
    __ Mov(declaredNumArgs, callField);
    __ Lsr(declaredNumArgs, declaredNumArgs, Method::NumArgsBits::START_BIT);
    __ And(declaredNumArgs.W(), declaredNumArgs.W(), LogicalImmediate::Create(
        Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT, W_REG_SIZE));
}

void AsmInterpreterCall::PushAsmInterpEntryFrame(ExtendedAssembler *assembler)
{
    Register glue = __ GlueRegister();

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
    __ Stp(glue, xzr, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // pc
    if (!assembler->FromInterpreterHandler()) {
        size_t end = __ GetCurrentPosition();
        if ((end - begin) != FrameCompletionPos::ARM64CppToAsmInterp) {
            LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::ARM64CppToAsmInterp
                                << "This frame has been modified, and the offset CppToAsmInterp should be updated too.";
        }
    }
    __ Add(fp, sp, Immediate(4 * FRAME_SLOT_SIZE));  // 4: 32 means skip frame type, prevSp, pc and glue
}

void AsmInterpreterCall::PopAsmInterpEntryFrame(ExtendedAssembler *assembler)
{
    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register prevFrameRegister = __ TempRegister1();
    [[maybe_unused]] TempRegister2Scope scope2(assembler);
    Register glue = __ TempRegister2();
    // 2: glue & pc
    __ Ldp(glue, xzr, MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Ldr(prevFrameRegister, MemoryOperand(sp, 0));
    __ Str(prevFrameRegister, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    // 2: skip frame type & prev
    __ Ldp(prevFrameRegister, xzr, MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
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
    __ Str(xzr, MemoryOperand(currentSlotRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
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
    Register &callTarget, Register &method, Register &pc, Register &temp)
{
    // prepare call entry
    __ Mov(x19, glue);    // X19 - glue
    __ Mov(fp, newSp);    // FP - sp
    __ Mov(x20, pc);      // X20 - pc
    __ Ldr(x21, MemoryOperand(method, Method::CONSTANT_POOL_OFFSET));   // X21 - constantpool
    __ Ldr(x22, MemoryOperand(callTarget, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    __ Ldr(x22, MemoryOperand(x22, ProfileTypeInfoCell::VALUE_OFFSET));  // X22 - profileTypeInfo
    __ Mov(x23, Immediate(JSTaggedValue::Hole().GetRawData()));                   // X23 - acc
    __ Ldr(x24, MemoryOperand(method, Method::LITERAL_INFO_OFFSET)); // X24 - hotnessCounter

    // call the first bytecode handler
    __ Ldrb(temp.W(), MemoryOperand(pc, 0));
    // 3 : 3 means *8
    __ Add(temp, glue, Operand(temp.W(), UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Ldr(temp, MemoryOperand(temp, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ UpdateGlueAndReadBarrier(glue);
    __ Br(temp);
}

void AsmInterpreterCall::CallNativeEntry(ExtendedAssembler *assembler, bool isJSFunction)
{
    Label callFastBuiltin;
    Label callNativeBuiltin;
    Register glue = x0;
    Register argv = x5;
    Register function = x1;
    Register nativeCode = x7;
    Register temp = x9;
    // get native pointer
    if (isJSFunction) {
        Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
        __ Ldr(nativeCode, MemoryOperand(function, JSFunctionBase::CODE_ENTRY_OFFSET));
        __ Tbnz(callFieldRegister, Method::IsFastBuiltinBit::START_BIT, &callFastBuiltin);
    } else {
        // JSProxy or JSBoundFunction
        Register method = x2;
        __ Ldr(nativeCode, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    }

    __ Bind(&callNativeBuiltin);
    // For non-FastBuiltin native JSFunction, Call will enter C++ and GlobalEnv needs to be set on glue
    if (isJSFunction) {
        Register lexicalEnv = temp;
        Label next;
        __ Ldr(lexicalEnv, MemoryOperand(function, JSFunction::LEXICAL_ENV_OFFSET));
        __ Cmp(lexicalEnv, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(Condition::EQ, &next);
        __ Str(lexicalEnv, MemoryOperand(glue, JSThread::GlueData::GetCurrentEnvOffset(false)));
        __ Bind(&next);
    }
    // 2: function & align
    __ Stp(function, xzr, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2: skip argc & thread
    __ Sub(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_ENTRY_FRAME, temp, argv);
    __ Mov(temp, argv);
#ifdef ENABLE_CMC_IR_FIX_REGISTER
    Register calleeSaveGlue = x28;
    __ Mov(calleeSaveGlue, glue); // move glue to a callee-save register
#endif
    __ Sub(x0, temp, Immediate(2 * FRAME_SLOT_SIZE));  // 2: skip argc & thread
    CallNativeInternal(assembler, nativeCode);

    // 4: skip function
    __ Add(sp, sp, Immediate(4 * FRAME_SLOT_SIZE));
    __ Ret();

    __ Bind(&callFastBuiltin);
    CallFastBuiltin(assembler, &callNativeBuiltin);
}

// InterpreterEntry attempts to call a fast builtin. Entry registers:
// Input: glue           - %X0
//        callTarget     - %X1
//        method         - %X2
//        callField      - %X3
//        argc           - %X4
//        argv           - %X5(<callTarget, newTarget, this> are at the beginning of argv)
//        nativeCode     - %7
// Fast builtin uses C calling convention:
// Input: glue           - %X0
//        nativeCode     - %X1
//        func           - %X2
//        newTarget      - %X3
//        this           - %X4
//        argc           - %X5
//        arg0           - %X6
//        arg1           - %X7
//        arg2           - stack
void AsmInterpreterCall::CallFastBuiltin(ExtendedAssembler *assembler, Label *callNativeBuiltin)
{
    Label dispatchTable[3]; // 3: call with argc = 0, 1, 2
    Label callEntryAndRet;
    Register glue = x0;
    Register function = x1;
    Register method = x2;
    Register argc = x4;
    Register argv = x5;
    Register nativeCode = x7;

    Register builtinId = __ AvailableRegister1();
    Register temp = __ AvailableRegister2();
    // Get builtinId
    __ Ldr(builtinId, MemoryOperand(method, Method::EXTRA_LITERAL_INFO_OFFSET));
    __ Lsr(builtinId.W(), builtinId.W(), Method::BuiltinIdBits::START_BIT);
    __ And(builtinId.W(), builtinId.W(),
           LogicalImmediate::Create((1LU << Method::BuiltinIdBits::SIZE) - 1, W_REG_SIZE));
    __ Cmp(builtinId.W(), Immediate(BUILTINS_STUB_ID(BUILTINS_CONSTRUCTOR_STUB_FIRST)));
    __ B(Condition::GE, callNativeBuiltin);

    __ Cmp(argc, Immediate(3)); // 3: Quick arity check: we only handle argc <= 3 here
    __ B(Condition::HI, callNativeBuiltin);

    // Resolve stub entry pointer: glue->builtinsStubEntries[builtinId]
    __ Add(builtinId, glue, Operand(builtinId.W(), UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Ldr(builtinId, MemoryOperand(builtinId, JSThread::GlueData::GetBuiltinsStubEntriesOffset(false)));
    // Create AsmBridge frame
    PushAsmBridgeFrame(assembler);
    // Shuffle registers to match C calling convention, X0(glue) already in place
    __ Mov(temp, function); // Save function to temp
    __ Mov(x1, nativeCode); // X1 = nativeCode
    __ Mov(x2, temp); // X2 = func
    __ Mov(temp, argv); // temp = argv
    __ Mov(x5, argc); // X5 = argc
    __ Ldr(x3, MemoryOperand(temp, FRAME_SLOT_SIZE)); // X3 = newTarget
    __ Ldr(x4, MemoryOperand(temp, DOUBLE_SLOT_SIZE)); // X4 = this

    // Dispatch according to argc (0, 1, 2, or 3)
    __ Cmp(x5, Immediate(0)); // 0: acgc = 0
    __ B(Condition::EQ, &dispatchTable[0]); // 0: acgc = 0
    __ Cmp(x5, Immediate(1)); // 1: acgc = 1
    __ B(Condition::EQ, &dispatchTable[1]); // 1: acgc = 1
    __ Cmp(x5, Immediate(2)); // 2: acgc = 2
    __ B(Condition::EQ, &dispatchTable[2]); // 2: acgc = 2
    // fallthrough to argc = 3

    // argc = 3
    __ Ldr(x7, MemoryOperand(temp, QUINTUPLE_SLOT_SIZE));
    __ Stp(x7, x7, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
    __ Ldp(x6, x7, MemoryOperand(temp, TRIPLE_SLOT_SIZE));
    __ B(&callEntryAndRet);

    // argc = 0
    __ Bind(&dispatchTable[0]);
    {
        __ Mov(x6, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Stp(x7, x7, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
        __ B(&callEntryAndRet);
    }
    // argc = 1
    __ Bind(&dispatchTable[1]);
    {
        __ Ldr(x6, MemoryOperand(temp, TRIPLE_SLOT_SIZE));
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Stp(x7, x7, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
        __ B(&callEntryAndRet);
    }
    // argc = 2
    __ Bind(&dispatchTable[2]);
    {
        __ Mov(x7, Immediate(JSTaggedValue::VALUE_UNDEFINED)); // dummy for stack slot
        __ Stp(x7, x7, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX));
        __ Ldp(x6, x7, MemoryOperand(temp, TRIPLE_SLOT_SIZE));
        // fallthrough to callEntryAndRet
    }

    __ Bind(&callEntryAndRet);
    {
        __ Blr(builtinId);
        __ Add(sp, sp, Immediate(DOUBLE_SLOT_SIZE));
        // Tear down frame and return
        PopAsmBridgeFrame(assembler);
        __ Ret();
    }
}

void AsmInterpreterCall::ThrowStackOverflowExceptionAndReturn(ExtendedAssembler *assembler, Register glue,
    Register fpReg, Register op)
{
    if (fpReg != sp) {
        __ Mov(sp, fpReg);
    }
    __ Mov(op, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException));
    // 3 : 3 means *8
    __ Add(op, glue, Operand(op, LSL, 3));
    __ Ldr(op, MemoryOperand(op, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    if (glue != x0) {
        __ Mov(x0, glue);
    }

#ifdef ENABLE_CMC_IR_FIX_REGISTER
    __ Mov(x28, glue); // move glue to a callee-save register
#endif
    __ Blr(op);
    __ UpdateGlueAndReadBarrier();
    __ RestoreFpAndLr();
    __ Ret();
}

void AsmInterpreterCall::ThrowStackOverflowExceptionAndReturnToAsmInterpBridgeFrame(ExtendedAssembler *assembler,
    Register glue, Register fpReg, Register op)
{
    if (fpReg != sp) {
        __ Mov(sp, fpReg);
    }

    if (glue != x0) {
        __ Mov(x0, glue);
    }

    __ PushFpAndLr();
    __ Mov(op, Immediate(static_cast<int64_t>(FrameType::ASM_BRIDGE_FRAME)));
    __ Stp(x10, op, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX)); // frame type and caller save
    __ Add(fp, sp, Immediate(DOUBLE_SLOT_SIZE));

    __ Mov(op, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException));
    __ Stp(op, xzr, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX)); // argc and runtime id
    __ Mov(x10, Immediate(kungfu::RuntimeStubCSigns::ID_CallRuntime));
    // 3 : 3 means *8
    __ Add(x10, glue, Operand(x10, LSL, 3));
    __ Ldr(x10, MemoryOperand(x10, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Blr(x10);
    // 2: skip argc and runtime_id
    __ Add(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    __ Ldr(x10, MemoryOperand(sp, FRAME_SLOT_SIZE, POSTINDEX));

    __ Mov(sp, fp);
    __ RestoreFpAndLr();

    // +----------------------------------------------------+
    // |                     return addr                    |
    // |----------------------------------------------------| <---- FP
    // |                     frame type                     |           ^                       ^
    // |----------------------------------------------------|           |                       |
    // |                     prevSp                         |           |                       |
    // |----------------------------------------------------|           |                       |
    // |                     pc                             |           |                       |
    // |----------------------------------------------------|  PushAsmInterpBridgeFrame     total skip
    // |      18 callee save regs(x19 - x28, v8 - v15)      |           |                       |
    // |----------------------------------------------------|           v                       v
    // |                     lr                 		    |
    // +----------------------------------------------------+
    // Base on PushAsmInterpBridgeFrame, need to skip AsmInterpBridgeFrame size and callee Save Registers(18)
    // but no lr(-1), x64 should skip lr because Ret in x64 will set stack pointer += 8
    int32_t skipNum = static_cast<int32_t>(AsmInterpretedBridgeFrame::GetSize(false)) / FRAME_SLOT_SIZE + 18 - 1;
    __ Add(sp, fp, Immediate(-skipNum * FRAME_SLOT_SIZE));
    __ Ret();
}
#undef __
}  // panda::ecmascript::aarch64
