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

#include "ecmascript/compiler/trampoline/aarch64/common_call.h"

#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"

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
#ifdef USE_CMC_GC
    Register tempRegisterW(X17, W); // can not be used to store any variable
    __ Ldr(tempRegisterW, MemoryOperand(callTargetRegister, TaggedObject::HCLASS_OFFSET));
    Register baseAddrRegister(X16);
    __ Ldr(baseAddrRegister, MemoryOperand(glueRegister, JSThread::GlueData::GetBaseAddressOffset(false)));
    __ Add(tempRegister, tempRegister, baseAddrRegister);
#else
    __ Ldr(tempRegister, MemoryOperand(callTargetRegister, TaggedObject::HCLASS_OFFSET));
#endif
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
        CallNativeEntry(assembler, true);
    }
    __ Bind(&callNativeEntry);
    CallNativeEntry(assembler, false);
    __ Bind(&callJSFunctionEntry);
    {
        __ Tbnz(callFieldRegister, MethodLiteral::IsNativeBit::START_BIT, &callNativeEntry);
        // fast path
        __ Add(argvRegister, argvRegister, Immediate(NUM_MANDATORY_JSFUNC_ARGS * JSTaggedValue::TaggedTypeSize()));
        JSCallCommonEntry(assembler, JSCallMode::CALL_ENTRY, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
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

void AsmInterpreterCall::JSCallCommonEntry(ExtendedAssembler *assembler,
    JSCallMode mode, FrameTransitionType type)
{
    Label stackOverflow;
    Register glueRegister = __ GlueRegister();
    Register fpRegister = __ AvailableRegister1();
    Register currentSlotRegister = __ AvailableRegister3();
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
    if (!kungfu::AssemblerModule::IsJumpToCallCommonEntry(mode) || type == FrameTransitionType::BASELINE_TO_OTHER ||
        type == FrameTransitionType::BASELINE_TO_BASELINE_CHECK) {
        __ PushFpAndLr();
    }
    // save fp
    __ Mov(fpRegister, Register(SP));
    __ Mov(currentSlotRegister, Register(SP));

    {
        // Reserve enough sp space to prevent stack parameters from being covered by cpu profiler.
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register tempRegister = __ TempRegister1();
        __ Ldr(tempRegister, MemoryOperand(glueRegister, JSThread::GlueData::GetStackLimitOffset(false)));
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
    PushCallThis(assembler, mode, &stackOverflow, type);
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
        __ Ldr(Register(X22), MemoryOperand(temp, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
        __ Ldr(Register(X22), MemoryOperand(Register(X22), ProfileTypeInfoCell::VALUE_OFFSET));
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
    return INVALID_REG;
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
    Register glue(X0);
    Register nativeCode(X1);
    Register callTarget(X2);
    Register thisObj(X3);
    Register argc(X4);
    Register argv(X5);
    Register newTarget(X6);
    Register opArgc(X8);
    Register opArgv(X9);
    Register temp(X10);
    Register currentSlotRegister(X11);
    Register spRegister(SP);

    Label pushThis;
    Label stackOverflow;
    bool isFrameComplete = PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME_WITH_ARGV, temp, argc);

    __ Mov(currentSlotRegister, spRegister);
    // Reserve enough sp space to prevent stack parameters from being covered by cpu profiler.
    __ Ldr(temp, MemoryOperand(glue, JSThread::GlueData::GetStackLimitOffset(false)));
    __ Mov(Register(SP), temp);

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
        __ Add(Register(FP), temp, Operand(argc, LSL, 3));  // 3: argc * 8
    }

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
        __ Mov(temp, spRegister);
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
        __ Add(Register(FP), temp, Immediate(FRAME_SLOT_SIZE * 6));

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

bool AsmInterpreterCall::PushBuiltinFrame(ExtendedAssembler *assembler, Register glue,
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
        // 2: -2 * FRAME_SLOT_SIZE means type & next
        __ Stp(next, op, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        // 2: 2 * FRAME_SLOT_SIZE means skip next and frame type
        __ Add(Register(FP), sp, Immediate(2 * FRAME_SLOT_SIZE));
        return true;
    } else if (type == FrameType::BUILTIN_ENTRY_FRAME) {
        // 2: -2 * FRAME_SLOT_SIZE means type & next
        __ Stp(next, op, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        // 2: 2 * FRAME_SLOT_SIZE means skip next and frame type
        __ Add(Register(FP), sp, Immediate(2 * FRAME_SLOT_SIZE));
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
#ifdef USE_CMC_GC
        Register tempW(X7, W);
        __ Ldr(tempW, MemoryOperand(ret, TaggedObject::HCLASS_OFFSET));
        Register baseAddrRegister(X16);
        __ Ldr(baseAddrRegister, MemoryOperand(glueRegister, JSThread::GlueData::GetBaseAddressOffset(false)));
        __ Add(temp, temp, baseAddrRegister);
#else
        __ Ldr(temp, MemoryOperand(ret, TaggedObject::HCLASS_OFFSET));
#endif
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
    Register glue(X19);
    Register rsp(SP);
    Register currentSp(X21);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register fpRegister = __ TempRegister1();
    int64_t fpOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false)) -
        static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(fpOffset < 0);
    __ Ldur(fpRegister, MemoryOperand(currentSp, fpOffset));
    __ Mov(rsp, fpRegister);
    __ RestoreFpAndLr();
    __ Mov(Register(X0), Register(FP));

    // Check and set result
    Register ret = X0;
    Register jumpSizeRegister = X22;
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
            Register temp = X19;
            Label notEcmaObject;
            __ Mov(temp, Immediate(JSTaggedValue::TAG_HEAPOBJECT_MASK));
            __ And(temp, temp, ret);
            __ Cmp(temp, Immediate(0));
            __ B(Condition::NE, &notEcmaObject);
            // acc is heap object
#ifdef USE_CMC_GC
            Register tempW(X19, W);
            __ Ldr(tempW, MemoryOperand(ret, TaggedObject::HCLASS_OFFSET));
            Register baseAddrRegister(X16);
            __ Ldr(baseAddrRegister, MemoryOperand(glue, JSThread::GlueData::GetBaseAddressOffset(false)));
            __ Add(temp, temp, baseAddrRegister);
#else
            __ Ldr(temp, MemoryOperand(ret, TaggedObject::HCLASS_OFFSET));
#endif
            __ Ldr(temp, MemoryOperand(temp, JSHClass::BIT_FIELD_OFFSET));
            __ And(temp.W(), temp.W(), LogicalImmediate::Create(0xFF, RegWSize));
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
                __ Lsr(temp.W(), temp.W(), MethodLiteral::FunctionKindBits::START_BIT);
                __ And(temp.W(), temp.W(),
                       LogicalImmediate::Create((1LU << MethodLiteral::FunctionKindBits::SIZE) - 1, RegWSize));
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

// c++ calling convention
// X0 - glue
// X1 - callTarget
// X2 - method
// X3 - callField
// X4 - receiver
// X5 - value
void AsmInterpreterCall::CallGetterToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallGetterToBaseline));
    Label target;

    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_GETTER, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
    }
}

void AsmInterpreterCall::CallSetterToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallSetterToBaseline));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_SETTER, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
    }
}

void AsmInterpreterCall::CallContainersArgs2ToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallContainersArgs2ToBaseline));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG2_WITH_RETURN,
                          FrameTransitionType::OTHER_TO_BASELINE_CHECK);
    }
}

void AsmInterpreterCall::CallContainersArgs3ToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallContainersArgs3ToBaseline));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG3_WITH_RETURN,
                          FrameTransitionType::OTHER_TO_BASELINE_CHECK);
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
void AsmInterpreterCall::CallReturnWithArgvToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallReturnWithArgvToBaseline));
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
        __ Stp(X29, X30, MemoryOperand(SP, -DOUBLE_SLOT_SIZE, PREINDEX));
        // Zero register means OPTIMIZED_FRAME
        __ Stp(X0, Zero, MemoryOperand(SP, -DOUBLE_SLOT_SIZE, PREINDEX));
        __ Add(FP, SP, Immediate(DOUBLE_SLOT_SIZE));
    }
    int32_t PreserveRegPairIndex = 9;
    // x0~x14,x16,x17,x18 should be preserved,
    // other general registers are callee saved register, callee will save them.
    __ Sub(SP, SP, Immediate(DOUBLE_SLOT_SIZE * PreserveRegPairIndex));
    __ Stp(X1, X2, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(X3, X4, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(X5, X6, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(X7, X8, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(X9, X10, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(X11, X12, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(X13, X14, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Stp(X16, X17, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (--PreserveRegPairIndex)));
    __ Str(X18, MemoryOperand(SP, FRAME_SLOT_SIZE));
    __ Blr(X15);
    __ Ldr(X18, MemoryOperand(SP, FRAME_SLOT_SIZE));
    __ Ldp(X16, X17, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(X13, X14, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(X11, X12, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(X9, X10, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(X7, X8, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(X5, X6, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(X3, X4, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldp(X1, X2, MemoryOperand(SP, DOUBLE_SLOT_SIZE * (PreserveRegPairIndex++)));
    __ Ldr(X0, MemoryOperand(SP, DOUBLE_SLOT_SIZE * PreserveRegPairIndex));
    {
        // epilogue to restore sp, fp, lr.
        // Skip x0 slot and frametype slot
        __ Add(SP, SP, Immediate(DOUBLE_SLOT_SIZE * PreserveRegPairIndex +
            FRAME_SLOT_SIZE + FRAME_SLOT_SIZE));
        __ Ldp(FP, X30, MemoryOperand(SP, DOUBLE_SLOT_SIZE, AddrMode::POSTINDEX));
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

#ifdef USE_CMC_GC
    __ Ret();
#else
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

        __ And(X15, X3, LogicalImmediate::Create(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK), RegXSize));
        // X15 is the region address of value.
        __ Ldrb(Register(X15, W), MemoryOperand(X15, 0));
        // X15 is the flag load from region of value.
        __ Cmp(Register(X15, W), Immediate(SHARED_SWEEPABLE_SPACE_BEGIN));
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

        __ Cmp(Register(X15, W), Immediate(RegionSpaceFlag::IN_YOUNG_SPACE));
        __ B(NE, &checkMark);
        // if value is not in young, goto checkMark

        __ And(X15, X1, LogicalImmediate::Create(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK), RegXSize));
        // X15 is the region address of obj.
        __ Ldrb(Register(X15, W), MemoryOperand(X15, 0));
        // X15 is the flag load from region of obj.
        __ Cmp(Register(X15, W), Immediate(RegionSpaceFlag::IN_YOUNG_SPACE));
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

        __ Mov(X15, JSThread::GlueData::GetGCStateBitFieldOffset(false));
        __ Ldrb(Register(X15, W), MemoryOperand(X0, Register(X15), UXTX));
        __ Tst(Register(X15, W), LogicalImmediate::Create(JSThread::CONCURRENT_MARKING_BITFIELD_MASK, RegWSize));
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
        __ Cmp(Register(X15, W), Immediate(RegionSpaceFlag::IN_SHARED_READ_ONLY_SPACE));
        __ B(NE, &needShareBarrier);
        __ Ret();
    }

    __ Bind(&needCallNotShare);
    {
        int32_t NonSValueBarrier = static_cast<int32_t>(JSThread::GlueData::GetCOStubEntriesOffset(false)) +
            kungfu::CommonStubCSigns::SetNonSValueWithBarrier * FRAME_SLOT_SIZE;
        __ Mov(X15, NonSValueBarrier);
    }
    __ Bind(&needCall);
    {
        __ Ldr(X15, MemoryOperand(X0, Register(X15), UXTX));
        PreserveMostCall(assembler);
    }
    __ Bind(&needShareBarrier);
    {
        ASMFastSharedWriteBarrier(assembler, needCall);
    }
#endif // USE_CMC_GC
}

// %x0 - glue
// %x1 - obj
// %x2 - offset
// %x3 - value
void AsmInterpreterCall::ASMFastSharedWriteBarrier(ExtendedAssembler* assembler, Label& needCall)
{
#ifdef USE_CMC_GC
    (void)assembler;
    (void)needCall;
    __ Ret();
#else
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
        __ And(X15, X1, LogicalImmediate::Create(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK), RegXSize));
        __ Ldrb(Register(X15, W), MemoryOperand(X15, 0));
        // X15 is the flag load from region of obj.
        __ Cmp(Register(X15, W), Immediate(RegionSpaceFlag::SHARED_SPACE_BEGIN));
        __ B(GE, &checkBarrierForSharedValue);  // if objflag >= SHARED_SPACE_BEGIN  => checkBarrierForSharedValue
    }
    {
        // int8_t *objRegion = obj & (~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK))
        // int8_t *localToShareSet = *(objRegion + LocalToShareSetOffset)
        // if (localToShareSet == 0){
        //    goto callSharedBarrier
        // }
        __ And(X15, X1, LogicalImmediate::Create(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK), RegXSize));
        __ Ldr(X15, MemoryOperand(X15, Region::PackedData::GetLocalToShareSetOffset(false)));
        // X15 is localToShareSet for obj region.
        __ Cbz({X15, X}, &callSharedBarrier);   // if localToShareSet == 0  => callSharedBarrier
    }
    {
        // X16, X17 will be used as scratch register, spill them.
        // the caller will call this function with inline asm, it will not save any registers except x15.
        // So we need spill and restore x16, x17 when we need them as scratch register.
        {
            __ Stp(X16, X17, MemoryOperand(SP, -DOUBLE_SLOT_SIZE, PREINDEX));
        }
        // int64_t objOffset = obj & DEFAULT_REGION_MASK
        // int64_t slotOffset = objOffset + offset
        __ And(X16, X1, LogicalImmediate::Create(DEFAULT_REGION_MASK, RegXSize));
        __ Add(X16, X16, Operand(Register(X2)));

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
        __ Ubfm(X17, X16, TAGGED_TYPE_SIZE_LOG, TAGGED_TYPE_SIZE_LOG + GCBitset::BIT_PER_WORD_LOG2 - 1);

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
        __ Ubfm(X16, X16, TAGGED_TYPE_SIZE_LOG + GCBitset::BIT_PER_WORD_LOG2,
                sizeof(uint32_t) * GCBitset::BIT_PER_BYTE + TAGGED_TYPE_SIZE_LOG - 1);
        __ Add(X15, X15, Operand(Register(X16), LSL, GCBitset::BYTE_PER_WORD_LOG2));
        __ Add(X15, X15, Immediate(RememberedSet::GCBITSET_DATA_OFFSET));
        // X15 is the address of bitset value. X15 = X15 + X16 << BYTE_PER_WORD_LOG2 + GCBITSET_DATA_OFFSET

        // mask = 1 << indexInWord
        __ Mov(Register(X16, W), 1);
        __ Lsl(Register(X17, W), Register(X16, W), Register(X17, W)); // X17 is the mask

        __ Ldr(Register(X16, W), MemoryOperand(X15, 0)); // x16: oldsetValue
        __ Tst(Register(X16, W), Register(X17, W));
        __ B(NE, &restoreScratchRegister);
        __ Orr(Register(X16, W), Register(X16, W), Register(X17, W));
        __ Str(Register(X16, W), MemoryOperand(X15, 0));
    }
    __ Bind(&restoreScratchRegister);
    {
        __ Ldp(X16, X17, MemoryOperand(SP, DOUBLE_SLOT_SIZE, POSTINDEX));
    }
    __ Bind(&checkBarrierForSharedValue);
    {
        // checkBarrierForSharedValue:
        // int8_t GCStateBitField = *(glue+SharedGCStateBitFieldOffset)
        // if (GCStateBitField & JSThread::SHARED_CONCURRENT_MARKING_BITFIELD_MASK != 0) {
        //    goto callSharedBarrier
        // }
        // return
        __ Mov(X15, JSThread::GlueData::GetSharedGCStateBitFieldOffset(false));
        __ Ldrb(Register(X15, W), MemoryOperand(X0, Register(X15), UXTX));
        static_assert(JSThread::SHARED_CONCURRENT_MARKING_BITFIELD_MASK == 1 && "Tbnz can't handle other bit mask");
        __ Tbnz(Register(X15, W), 0, &callSharedBarrier);
        // if GCState is not READY_TO_MARK, go to needCallNotShare.
        __ Ret();
    }

    __ Bind(&callSharedBarrier);
    {
        int32_t SValueBarrierOffset = static_cast<int32_t>(JSThread::GlueData::GetCOStubEntriesOffset(false)) +
            kungfu::CommonStubCSigns::SetSValueWithBarrier * FRAME_SLOT_SIZE;
        __ Mov(X15, SValueBarrierOffset);
        __ B(&needCall);
    }
#endif // USE_CMC_GC
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
    __ Ldr(temp, MemoryOperand(glue, JSThread::GlueData::GetStackLimitOffset(false)));
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
        __ Mov(Register(SP), currentSlotRegister);
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
            if (glue != X19) {
                __ Mov(X19, glue);
            }
            if (methodRegister != X21) {
                __ Mov(X21, methodRegister);
            }
            __ Mov(currentSlotRegister, Immediate(BASELINEJIT_PC_FLAG));
            // -3: frame type, prevSp, pc
            __ Stur(currentSlotRegister, MemoryOperand(newSpRegister, -3 * FRAME_SLOT_SIZE));
            __ Mov(Register(X29), newSpRegister);
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

    if (glueRegister.GetId() != X19) {
        __ Mov(Register(X19), glueRegister);
    }
    __ Ldrh(Register(X24, W), MemoryOperand(methodRegister, Method::LITERAL_INFO_OFFSET));
    if (accRegister == INVALID_REG) {
        __ Mov(Register(X23), Immediate(JSTaggedValue::VALUE_HOLE));
    } else {
        ASSERT(accRegister == Register(X23));
    }
    __ Ldr(Register(X22), MemoryOperand(callTargetRegister, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    __ Ldr(Register(X22), MemoryOperand(Register(X22), ProfileTypeInfoCell::VALUE_OFFSET));
    __ Ldr(Register(X21), MemoryOperand(methodRegister, Method::CONSTANT_POOL_OFFSET));
    __ Mov(Register(X20), pcRegister);
    __ Mov(Register(FP), newSpRegister);

    Register bcIndexRegister = __ AvailableRegister1();
    Register tempRegister = __ AvailableRegister2();
    if (hasException) {
        __ Mov(bcIndexRegister.W(), Immediate(kungfu::BytecodeStubCSigns::ID_ExceptionHandler));
    } else {
        __ Ldrb(bcIndexRegister.W(), MemoryOperand(pcRegister, 0));
    }
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
    __ Add(fp, sp, Immediate(4 * FRAME_SLOT_SIZE));  // 4: 32 means skip frame type, prevSp, pc and glue
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
    Register &callTarget, Register &method, Register &pc, Register &temp)
{
    // prepare call entry
    __ Mov(Register(X19), glue);    // X19 - glue
    __ Mov(Register(FP), newSp);    // FP - sp
    __ Mov(Register(X20), pc);      // X20 - pc
    __ Ldr(Register(X21), MemoryOperand(method, Method::CONSTANT_POOL_OFFSET));   // X21 - constantpool
    __ Ldr(Register(X22), MemoryOperand(callTarget, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    __ Ldr(Register(X22), MemoryOperand(Register(X22), ProfileTypeInfoCell::VALUE_OFFSET));  // X22 - profileTypeInfo
    __ Mov(Register(X23), Immediate(JSTaggedValue::Hole().GetRawData()));                   // X23 - acc
    __ Ldr(Register(X24), MemoryOperand(method, Method::LITERAL_INFO_OFFSET)); // X24 - hotnessCounter

    // call the first bytecode handler
    __ Ldrb(temp.W(), MemoryOperand(pc, 0));
    // 3 : 3 means *8
    __ Add(temp, glue, Operand(temp.W(), UXTW, FRAME_SLOT_SIZE_LOG2));
    __ Ldr(temp, MemoryOperand(temp, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(temp);
}

void AsmInterpreterCall::CallNativeEntry(ExtendedAssembler *assembler, bool isJsProxy)
{
    Register glue(X0);
    Register argv(X5);
    Register function(X1);
    Register nativeCode(X7);
    Register temp(X9);
    // get native pointer
    if (isJsProxy) {
        Register method(X2);
        __ Ldr(nativeCode, MemoryOperand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    } else {
        __ Ldr(nativeCode, MemoryOperand(function, JSFunctionBase::CODE_ENTRY_OFFSET));
    }

    Register sp(SP);
    // 2: function & align
    __ Stp(function, Register(Zero), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2: skip argc & thread
    __ Sub(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_ENTRY_FRAME, temp, argv);
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
    Register sp(SP);

    if (fp != sp) {
        __ Mov(sp, fp);
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

void AsmInterpreterCall::ThrowStackOverflowExceptionAndReturnToAsmInterpBridgeFrame(ExtendedAssembler *assembler,
    Register glue, Register fp, Register op)
{
    Register sp(SP);

    if (fp != sp) {
        __ Mov(sp, fp);
    }

    if (glue.GetId() != X0) {
        __ Mov(Register(X0), glue);
    }
    
    __ PushFpAndLr();
    __ Mov(op, Immediate(static_cast<int64_t>(FrameType::ASM_BRIDGE_FRAME)));
    __ Stp(Register(X10), op, MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX)); // frame type and caller save
    __ Add(Register(FP), sp, Immediate(DOUBLE_SLOT_SIZE));
    
    __ Mov(op, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException));
    __ Stp(op, Register(Zero), MemoryOperand(sp, -DOUBLE_SLOT_SIZE, PREINDEX)); // argc and runtime id
    __ Mov(Register(X10), Immediate(kungfu::RuntimeStubCSigns::ID_CallRuntime));
    // 3 : 3 means *8
    __ Add(Register(X10), glue, Operand(Register(X10), LSL, 3));
    __ Ldr(Register(X10), MemoryOperand(Register(X10), JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Blr(Register(X10));
    // 2: skip argc and runtime_id
    __ Add(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    __ Ldr(Register(X10), MemoryOperand(sp, FRAME_SLOT_SIZE, POSTINDEX));

    __ Mov(sp, Register(FP));
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
    __ Add(Register(SP), Register(FP), Immediate(-skipNum * FRAME_SLOT_SIZE));
    __ Ret();
}
#undef __
}  // panda::ecmascript::aarch64
