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

#include "ecmascript/compiler/trampoline/x64/common_call.h"

#include "ecmascript/compiler/assembler/assembler.h"
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

namespace panda::ecmascript::x64 {
#define __ assembler->

// Generate code for Entering asm interpreter
// Input: glue           - %rdi
//        callTarget     - %rsi
//        method         - %rdx
//        callField      - %rcx
//        argc           - %r8
//        argv           - %r9(<callTarget, newTarget, this> are at the beginning of argv)
void AsmInterpreterCall::AsmInterpreterEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(AsmInterpreterEntry));
    Label target;
    // push asm interpreter entry frame
    size_t begin = __ GetCurrentPosition();
    PushAsmInterpEntryFrame(assembler);
    __ Callq(&target);
    PopAsmInterpEntryFrame(assembler);
    size_t end = __ GetCurrentPosition();
    if ((end - begin) != FrameCompletionPos::X64EntryFrameDuration) {
        LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::X64EntryFrameDuration
                            << "This frame has been modified, and the offset EntryFrameDuration should be updated too.";
    }
    __ Ret();

    __ Bind(&target);
    AsmInterpEntryDispatch(assembler);
}

// Generate code for generator re-enter asm interpreter
// c++ calling convention
// Input: %rdi - glue
//        %rsi - context(GeneratorContext)
void AsmInterpreterCall::GeneratorReEnterAsmInterp(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(GeneratorReEnterAsmInterp));
    Label target;
    size_t begin = __ GetCurrentPosition();
    PushAsmInterpEntryFrame(assembler);
    __ Callq(&target);
    PopAsmInterpEntryFrame(assembler);
    size_t end = __ GetCurrentPosition();
    if ((end - begin) != FrameCompletionPos::X64EntryFrameDuration) {
        LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::X64EntryFrameDuration
                            << "This frame has been modified, and the offset EntryFrameDuration should be updated too.";
    }
    __ Ret();

    __ Bind(&target);
    GeneratorReEnterAsmInterpDispatch(assembler);
}

void AsmInterpreterCall::GeneratorReEnterAsmInterpDispatch(ExtendedAssembler *assembler)
{
    Register glueRegister = __ GlueRegister();
    Register contextRegister = rsi;
    Register prevSpRegister = rbp;

    Register callTargetRegister = r9;
    Register methodRegister = rcx;
    Register tempRegister = r11;  // can not be used to store any variable
    Register opRegister = r8;  // can not be used to store any variable
    __ Movq(Operand(rsi, GeneratorContext::GENERATOR_METHOD_OFFSET), callTargetRegister);
    __ Movq(Operand(callTargetRegister, JSFunctionBase::METHOD_OFFSET), methodRegister);

    Label stackOverflow;

    Register fpRegister = r10;
    __ Movq(rsp, fpRegister);
    Register nRegsRegister = rdx;
    Register regsArrayRegister = r12;
    Register thisRegister = r15;
    // push context regs
    __ Movl(Operand(rsi, GeneratorContext::GENERATOR_NREGS_OFFSET), nRegsRegister);
    __ Movq(Operand(rsi, GeneratorContext::GENERATOR_THIS_OFFSET), thisRegister);
    __ Movq(Operand(rsi, GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET), regsArrayRegister);
    __ Addq(TaggedArray::DATA_OFFSET, regsArrayRegister);
    PushArgsWithArgvAndCheckStack(assembler, glueRegister, nRegsRegister, regsArrayRegister, tempRegister, opRegister,
        &stackOverflow);

    // newSp
    Register newSpRegister = r8;
    __ Movq(rsp, newSpRegister);

    // resume asm interp frame
    Register pcRegister = r12;
    PushGeneratorFrameState(assembler, prevSpRegister, fpRegister, callTargetRegister, thisRegister, methodRegister,
        contextRegister, pcRegister, tempRegister);

    // call bc stub
    DispatchCall(assembler, pcRegister, newSpRegister, methodRegister);
    __ Bind(&stackOverflow);
    {
        ThrowStackOverflowExceptionAndReturn(assembler, glueRegister, fpRegister, tempRegister);
    }
}

// Input: glue           - %rdi
//        callTarget     - %rsi
//        method         - %rdx
//        callField      - %rcx
//        argc           - %r8
//        argv           - %r9(<callTarget, newTarget, this> are at the beginning of argv)
//        prevSp         - %rbp
void AsmInterpreterCall::AsmInterpEntryDispatch(ExtendedAssembler *assembler)
{
    Label notJSFunction;
    Label callNativeEntry;
    Label callJSFunctionEntry;
    Label notCallable;
    Register glueRegister = rdi;
    Register callTargetRegister = rsi;
    Register argvRegister = r9;
    Register bitFieldRegister = r12;
    Register tempRegister = r11;  // can not be used to store any variable
    __ Movq(Operand(callTargetRegister, TaggedObject::HCLASS_OFFSET), tempRegister);  // hclass
    __ Movq(Operand(tempRegister, JSHClass::BIT_FIELD_OFFSET), bitFieldRegister);
    __ Cmpb(static_cast<int32_t>(JSType::JS_FUNCTION_FIRST), bitFieldRegister);
    __ Jb(&notJSFunction);
    __ Cmpb(static_cast<int32_t>(JSType::JS_FUNCTION_LAST), bitFieldRegister);
    __ Jbe(&callJSFunctionEntry);
    __ Bind(&notJSFunction);
    {
        __ Testq(static_cast<int64_t>(1ULL << JSHClass::CallableBit::START_BIT), bitFieldRegister);
        __ Jz(&notCallable);
        // fall through
    }
    __ Bind(&callNativeEntry);
    CallNativeEntry(assembler);
    __ Bind(&callJSFunctionEntry);
    {
        Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
        __ Btq(MethodLiteral::IsNativeBit::START_BIT, callFieldRegister);
        __ Jb(&callNativeEntry);

        __ Leaq(Operand(argvRegister, NUM_MANDATORY_JSFUNC_ARGS * JSTaggedValue::TaggedTypeSize()),
            argvRegister);
        JSCallCommonEntry(assembler, JSCallMode::CALL_ENTRY);
    }
    __ Bind(&notCallable);
    {
        __ Movq(glueRegister, rax);  // glue
        __ Pushq(0);                 // argc
        Register runtimeIdRegister = r12;
        __ Movq(kungfu::RuntimeStubCSigns::ID_ThrowNotCallableException, runtimeIdRegister);
        __ Pushq(runtimeIdRegister);  // runtimeId
        Register trampolineIdRegister = r12;
        Register trampolineRegister = r10;
        __ Movq(kungfu::RuntimeStubCSigns::ID_CallRuntime, trampolineIdRegister);
        __ Movq(Operand(rax, trampolineIdRegister, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)),
            trampolineRegister);
        __ Callq(trampolineRegister);
        __ Addq(16, rsp);  // 16: skip argc and runtime_id
        __ Ret();
    }
}

void AsmInterpreterCall::PushFrameState(ExtendedAssembler *assembler, Register prevSpRegister, Register fpRegister,
    Register callTargetRegister, Register thisRegister, Register methodRegister, Register pcRegister,
    Register operatorRegister)
{
    __ Pushq(static_cast<int32_t>(FrameType::ASM_INTERPRETER_FRAME));  // frame type
    __ Pushq(prevSpRegister);                                          // prevSp
    __ Movq(Operand(methodRegister, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET), pcRegister);
    __ Pushq(pcRegister);                                              // pc
    __ Pushq(fpRegister);                                              // fp
    __ Pushq(0);                                                       // jumpSizeAfterCall
    __ Movq(Operand(callTargetRegister, JSFunction::LEXICAL_ENV_OFFSET), operatorRegister);
    __ Pushq(operatorRegister);                                        // env
    __ Pushq(JSTaggedValue::Hole().GetRawData());                      // acc
    __ Pushq(thisRegister);                                            // thisObj
    __ Pushq(callTargetRegister);                                      // callTarget
}

void AsmInterpreterCall::PushGeneratorFrameState(ExtendedAssembler *assembler, Register prevSpRegister,
    Register fpRegister, Register callTargetRegister, Register thisRegister, Register methodRegister,
    Register contextRegister, Register pcRegister, Register operatorRegister)
{
    __ Pushq(static_cast<int32_t>(FrameType::ASM_INTERPRETER_FRAME));  // frame type
    __ Pushq(prevSpRegister);                                          // prevSp
    __ Movq(Operand(methodRegister, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET), pcRegister);
    __ Movl(Operand(contextRegister, GeneratorContext::GENERATOR_BC_OFFSET_OFFSET), operatorRegister);
    __ Addq(operatorRegister, pcRegister);
    __ Pushq(pcRegister);                                              // pc
    __ Pushq(fpRegister);                                              // fp
    __ Pushq(0);                                                       // jumpSizeAfterCall
    __ Movq(Operand(contextRegister, GeneratorContext::GENERATOR_LEXICALENV_OFFSET), operatorRegister);
    __ Pushq(operatorRegister);                                        // env
    __ Movq(Operand(contextRegister, GeneratorContext::GENERATOR_ACC_OFFSET), operatorRegister);
    __ Pushq(operatorRegister);                                        // acc
    __ Pushq(thisRegister);                                            // thisObj
    __ Pushq(callTargetRegister);                                      // callTarget
}

void AsmInterpreterCall::PushAsmInterpEntryFrame(ExtendedAssembler *assembler)
{
    size_t begin = __ GetCurrentPosition();
    if (!assembler->FromInterpreterHandler()) {
        __ PushCppCalleeSaveRegisters();
    }
    Register fpRegister = r10;
    __ Pushq(rdi);
    __ PushAlignBytes();
    __ Movq(Operand(rdi, JSThread::GlueData::GetLeaveFrameOffset(false)), fpRegister);
    // construct asm interpreter entry frame
    __ Pushq(rbp);
    __ Pushq(static_cast<int64_t>(FrameType::ASM_INTERPRETER_ENTRY_FRAME));
    __ Pushq(fpRegister);
    __ Pushq(0);    // pc
    if (!assembler->FromInterpreterHandler()) {
        size_t end = __ GetCurrentPosition();
        if ((end - begin) != FrameCompletionPos::X64CppToAsmInterp) {
            LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::X64CppToAsmInterp
                                << "This frame has been modified, and the offset CppToAsmInterp should be updated too.";
        }
    }
    __ Leaq(Operand(rsp, 3 * FRAME_SLOT_SIZE), rbp);  // 24: skip frame type, prevSp and pc
}

void AsmInterpreterCall::PopAsmInterpEntryFrame(ExtendedAssembler *assembler)
{
    __ Addq(8, rsp);   // 8: skip pc
    Register fpRegister = r10;
    __ Popq(fpRegister);
    __ Addq(FRAME_SLOT_SIZE, rsp);  // 8: skip frame type
    __ Popq(rbp);
    __ PopAlignBytes();
    __ Popq(rdi);
    __ Movq(fpRegister, Operand(rdi, JSThread::GlueData::GetLeaveFrameOffset(false)));
    size_t begin = __ GetCurrentPosition();
    if (!assembler->FromInterpreterHandler()) {
        __ PopCppCalleeSaveRegisters();
        size_t end = __ GetCurrentPosition();
        if ((end - begin) != FrameCompletionPos::X64AsmInterpToCpp) {
            LOG_COMPILER(FATAL) << (end - begin) << " != " << FrameCompletionPos::X64AsmInterpToCpp
                                << "This frame has been modified, and the offset AsmInterpToCp should be updated too.";
        }
    }
}

void AsmInterpreterCall::GetDeclaredNumArgsFromCallField(ExtendedAssembler *assembler, Register callFieldRegister,
    Register declaredNumArgsRegister)
{
    __ Movq(callFieldRegister, declaredNumArgsRegister);
    __ Shrq(MethodLiteral::NumArgsBits::START_BIT, declaredNumArgsRegister);
    __ Andq(MethodLiteral::NumArgsBits::Mask() >> MethodLiteral::NumArgsBits::START_BIT, declaredNumArgsRegister);
}

void AsmInterpreterCall::GetNumVregsFromCallField(ExtendedAssembler *assembler, Register callFieldRegister,
    Register numVregsRegister)
{
    __ Movq(callFieldRegister, numVregsRegister);
    __ Shrq(MethodLiteral::NumVregsBits::START_BIT, numVregsRegister);
    __ Andq(MethodLiteral::NumVregsBits::Mask() >> MethodLiteral::NumVregsBits::START_BIT, numVregsRegister);
}

void AsmInterpreterCall::JSCallCommonEntry(ExtendedAssembler *assembler, JSCallMode mode)
{
    Label stackOverflow;
    Register glueRegister = __ GlueRegister();
    Register fpRegister = __ AvailableRegister1();
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
    // save fp
    __ Movq(rsp, fpRegister);
    Register declaredNumArgsRegister = __ AvailableRegister2();
    GetDeclaredNumArgsFromCallField(assembler, callFieldRegister, declaredNumArgsRegister);

    Label slowPathEntry;
    Label fastPathEntry;
    Label pushCallThis;
    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    if (argc >= 0) {
        __ Cmpq(argc, declaredNumArgsRegister);
    } else {
        __ Cmpq(argcRegister, declaredNumArgsRegister);
    }
    __ Jne(&slowPathEntry);
    __ Bind(&fastPathEntry);
    JSCallCommonFastPath(assembler, mode, &stackOverflow);
    __ Bind(&pushCallThis);
    PushCallThis(assembler, mode, &stackOverflow);
    __ Bind(&slowPathEntry);
    JSCallCommonSlowPath(assembler, mode, &fastPathEntry, &pushCallThis, &stackOverflow);

    __ Bind(&stackOverflow);
    if (kungfu::AssemblerModule::IsJumpToCallCommonEntry(mode)) {
        __ Movq(fpRegister, rsp);
        Register tempRegister = __ AvailableRegister1();
        // only glue and acc are useful in exception handler
        if (glueRegister != r13) {
            __ Movq(glueRegister, r13);
        }
        Register acc = rsi;
        __ Movq(JSTaggedValue::VALUE_EXCEPTION, acc);
        Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);
        Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
        // Reload pc to make sure stack trace is right
        __ Movq(callTargetRegister, tempRegister);
        __ Movq(Operand(methodRegister, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET), r12);  // pc: r12
        // Reload constpool and profileInfo to make sure gc map work normally
        __ Movq(Operand(methodRegister, Method::PROFILE_TYPE_INFO_OFFSET), r14);       // profileTypeInfo: r14
        __ Movq(Operand(methodRegister, Method::CONSTANT_POOL_OFFSET), rbx);           // constantPool: rbx

        __ Movq(kungfu::BytecodeStubCSigns::ID_ThrowStackOverflowException, tempRegister);
        __ Movq(Operand(glueRegister, tempRegister, Times8, JSThread::GlueData::GetBCStubEntriesOffset(false)),
            tempRegister);
        __ Jmp(tempRegister);
    } else {
        [[maybe_unused]] TempRegisterScope scope(assembler);
        Register temp = __ TempRegister();
        ThrowStackOverflowExceptionAndReturn(assembler, glueRegister, fpRegister, temp);
    }
}

// void PushCallArgsxAndDispatch(uintptr_t glue, uintptr_t sp, uint64_t callTarget, uintptr_t method,
//     uint64_t callField, ...)
// GHC calling convention
// Input1: for callarg0/1/2/3         Input2: for callrange
// %r13 - glue                        // %r13 - glue
// %rbp - sp                          // %rbp - sp
// %r12 - callTarget                  // %r12 - callTarget
// %rbx - method                      // %rbx - method
// %r14 - callField                   // %r14 - callField
// %rsi - arg0                        // %rsi - actualArgc
// %rdi - arg1                        // %rdi - argv
// %r8  - arg2
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

void AsmInterpreterCall::JSCallCommonFastPath(ExtendedAssembler *assembler, JSCallMode mode, Label *stackOverflow)
{
    Register glueRegister = __ GlueRegister();
    Register arg0 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
    Register arg1 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);

    Label pushCallThis;
    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    // call range
    if (argc < 0) {
        Register argcRegister = arg0;
        Register argvRegister = arg1;
        __ Cmpq(0, argcRegister);
        __ Jbe(&pushCallThis);
        // fall through
        {
            [[maybe_unused]] TempRegisterScope scope(assembler);
            Register opRegister = __ TempRegister();
            Register op2Register = __ AvailableRegister2();
            PushArgsWithArgvAndCheckStack(assembler, glueRegister, argcRegister, argvRegister, opRegister, op2Register,
                stackOverflow);
        }
        __ Bind(&pushCallThis);
    } else if (argc > 0) {
        if (argc > 2) { // 2: call arg2
            if (mode == JSCallMode::CALL_THIS_ARG3_WITH_RETURN) {
                Register arg2 = __ CppJSCallAvailableRegister1();
                __ Pushq(arg2);
            } else {
                Register arg2 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
                __ Pushq(arg2);
            }
        }
        if (argc > 1) {
            __ Pushq(arg1);
        }
        if (argc > 0) {
            __ Pushq(arg0);
        }
    }
}

void AsmInterpreterCall::JSCallCommonSlowPath(ExtendedAssembler *assembler, JSCallMode mode,
                                              Label *fastPathEntry, Label *pushCallThis, Label *stackOverflow)
{
    Register glueRegister = __ GlueRegister();
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
    Register arg0 = argcRegister;
    Register arg1 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
    Label noExtraEntry;
    Label pushArgsEntry;

    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    Register declaredNumArgsRegister = __ AvailableRegister2();
    __ Testq(MethodLiteral::HaveExtraBit::Mask(), callFieldRegister);
    __ Jz(&noExtraEntry);
    // extra entry
    {
        [[maybe_unused]] TempRegisterScope scope(assembler);
        Register tempArgcRegister = __ TempRegister();
        if (argc >= 0) {
            __ PushArgc(argc, tempArgcRegister);
        } else {
            __ PushArgc(argcRegister, tempArgcRegister);
        }
    }
    __ Bind(&noExtraEntry);
    {
        if (argc == 0) {
            Register op1 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
            [[maybe_unused]] TempRegisterScope scope(assembler);
            Register op2 = __ TempRegister();
            PushUndefinedWithArgcAndCheckStack(assembler, glueRegister, declaredNumArgsRegister, op1, op2,
                stackOverflow);
            __ Jmp(fastPathEntry);
            return;
        }
        [[maybe_unused]] TempRegisterScope scope(assembler);
        Register diffRegister = __ TempRegister();
        __ Movq(declaredNumArgsRegister, diffRegister);
        if (argc >= 0) {
            __ Subq(argc, diffRegister);
        } else {
            __ Subq(argcRegister, diffRegister);
        }
        __ Cmpq(0, diffRegister);
        __ Jle(&pushArgsEntry);
        PushUndefinedWithArgc(assembler, diffRegister);
        __ Jmp(fastPathEntry);
    }
    __ Bind(&pushArgsEntry);
    __ Testq(MethodLiteral::HaveExtraBit::Mask(), callFieldRegister);
    __ Jnz(fastPathEntry);
    // arg1, declare must be 0
    if (argc == 1) {
        __ Jmp(pushCallThis);
        return;
    }
    // decalare < actual
    __ Cmpq(0, declaredNumArgsRegister);
    __ Je(pushCallThis);
    if (argc < 0) {
        Register argvRegister = arg1;
        [[maybe_unused]] TempRegisterScope scope(assembler);
        Register opRegister = __ TempRegister();
        PushArgsWithArgvAndCheckStack(assembler, glueRegister, declaredNumArgsRegister, argvRegister, opRegister,
            opRegister, stackOverflow);
    } else if (argc > 0) {
        Label pushArgs0;
        if (argc > 2) { // 2: call arg2
            // decalare is 2 or 1 now
            __ Cmpq(1, declaredNumArgsRegister);
            __ Je(&pushArgs0);
            __ Pushq(arg1);
        }
        if (argc > 1) {
            __ Bind(&pushArgs0);
            // decalare is is 1 now
            __ Pushq(arg0);
        }
    }
    __ Jmp(pushCallThis);
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
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
        case JSCallMode::CALL_THIS_ARG3:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG3);
        case JSCallMode::CALL_ENTRY:
        case JSCallMode::CALL_FROM_AOT: {
            Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
            __ Movq(Operand(argvRegister, -FRAME_SLOT_SIZE), defaultRegister);  // 8: this is just before the argv list
            return defaultRegister;
        }
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
            return __ CppJSCallAvailableRegister2();
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN: {
            return __ CppJSCallAvailableRegister1();
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return rInvalid;
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
            // -2: new Target offset
            __ Movq(Operand(argvRegister, -2 * FRAME_SLOT_SIZE), defaultRegister);
            return defaultRegister;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return rInvalid;
}

// Input: %r14 - callField
//        %rdi - argv
void AsmInterpreterCall::PushCallThis(ExtendedAssembler *assembler, JSCallMode mode, Label *stackOverflow)
{
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register thisRegister = __ AvailableRegister2();

    Label pushVregs;
    Label pushNewTarget;
    Label pushCallTarget;
    bool haveThis = kungfu::AssemblerModule::JSModeHaveThisArg(mode);
    bool haveNewTarget = kungfu::AssemblerModule::JSModeHaveNewTargetArg(mode);
    if (!haveThis) {
        __ Movq(JSTaggedValue::VALUE_UNDEFINED, thisRegister);  // default this: undefined
    } else {
        Register thisArgRegister = GetThisRegsiter(assembler, mode, thisRegister);
        if (thisRegister != thisArgRegister) {
            __ Movq(thisArgRegister, thisRegister);
        }
    }
    __ Testb(CALL_TYPE_MASK, callFieldRegister);
    __ Jz(&pushVregs);
    // fall through
    __ Testq(MethodLiteral::HaveThisBit::Mask(), callFieldRegister);
    __ Jz(&pushNewTarget);
    // push this
    if (!haveThis) {
        __ Pushq(JSTaggedValue::Undefined().GetRawData());
    } else {
        __ Pushq(thisRegister);
    }
    // fall through
    __ Bind(&pushNewTarget);
    {
        __ Testq(MethodLiteral::HaveNewTargetBit::Mask(), callFieldRegister);
        __ Jz(&pushCallTarget);
        if (!haveNewTarget) {
            __ Pushq(JSTaggedValue::Undefined().GetRawData());
        } else {
            [[maybe_unused]] TempRegisterScope scope(assembler);
            Register defaultRegister = __ TempRegister();
            Register newTargetRegister = GetNewTargetRegsiter(assembler, mode, defaultRegister);
            __ Pushq(newTargetRegister);
        }
    }
    // fall through
    __ Bind(&pushCallTarget);
    {
        __ Testq(MethodLiteral::HaveFuncBit::Mask(), callFieldRegister);
        __ Jz(&pushVregs);
        __ Pushq(callTargetRegister);
    }
    // fall through
    __ Bind(&pushVregs);
    {
        PushVregs(assembler, stackOverflow);
    }
}

// Input: %rbp - sp
//        %r12 - callTarget
//        %rbx - method
//        %r14 - callField
//        %rdx - jumpSizeAfterCall
//        %r10 - fp
void AsmInterpreterCall::PushVregs(ExtendedAssembler *assembler, Label *stackOverflow)
{
    Register glueRegister = __ GlueRegister();
    Register prevSpRegister = rbp;
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register fpRegister = __ AvailableRegister1();
    Register thisRegister = __ AvailableRegister2();

    Label pushFrameState;

    [[maybe_unused]] TempRegisterScope scope(assembler);
    Register tempRegister = __ TempRegister();
    // args register can reused now.
    Register pcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
    Register numVregsRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
    GetNumVregsFromCallField(assembler, callFieldRegister, numVregsRegister);
    __ Cmpq(0, numVregsRegister);
    __ Jz(&pushFrameState);
    Register temp2Register = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);  // reuse
    PushUndefinedWithArgcAndCheckStack(assembler, glueRegister, numVregsRegister, tempRegister, temp2Register,
        stackOverflow);
    // fall through
    Register newSpRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
    __ Bind(&pushFrameState);
    {
        __ Movq(rsp, newSpRegister);

        PushFrameState(assembler, prevSpRegister, fpRegister,
            callTargetRegister, thisRegister, methodRegister, pcRegister, tempRegister);
    }
    DispatchCall(assembler, pcRegister, newSpRegister, methodRegister);
}

// Input: %r13 - glue
//        %rbp - sp
//        %r12 - callTarget
//        %rbx - method
void AsmInterpreterCall::DispatchCall(ExtendedAssembler *assembler, Register pcRegister,
    Register newSpRegister, Register methodRegister, Register accRegister)
{
    Register glueRegister = __ GlueRegister();
    Label dispatchCall;
    // align 16 bytes
    __ Testq(15, rsp);  // 15: low 4 bits must be 0b0000
    __ Jnz(&dispatchCall);
    __ PushAlignBytes();
    __ Bind(&dispatchCall);
    // profileTypeInfo: r14
    __ Movq(Operand(methodRegister, Method::PROFILE_TYPE_INFO_OFFSET), r14);
    // glue may rdi
    if (glueRegister != r13) {
        __ Movq(glueRegister, r13);
    }
    // sp: rbp
    __ Movq(newSpRegister, rbp);
    // hotnessCounter: rdi
    __ Movzwq(Operand(methodRegister, Method::LITERAL_INFO_OFFSET), rdi);
    // constantPool: rbx
    __ Movq(Operand(methodRegister, Method::CONSTANT_POOL_OFFSET), rbx);
    // pc: r12
    if (pcRegister != r12) {
        __ Movq(pcRegister, r12);
    }

    Register bcIndexRegister = rax;
    Register tempRegister = __ AvailableRegister1();
    __ Movzbq(Operand(pcRegister, 0), bcIndexRegister);
    // acc: rsi
    if (accRegister != rInvalid) {
        ASSERT(accRegister == rsi);
    } else {
        __ Movq(JSTaggedValue::Hole().GetRawData(), rsi);
    }
    __ Movq(Operand(r13, bcIndexRegister, Times8, JSThread::GlueData::GetBCStubEntriesOffset(false)), tempRegister);
    __ Jmp(tempRegister);
}

// uint64_t PushCallRangeAndDispatchNative(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, uintptr_t argv[])
// c++ calling convention call js function
// Input: %rdi - glue
//        %rsi - nativeCode
//        %rdx - func
//        %rcx - thisValue
//        %r8  - argc
//        %r9  - argV (...)
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
    Register glue = rdi;
    Register nativeCode = rsi;
    Register func = rdx;
    Register thisValue = rcx;
    Register numArgs = r8;
    Register stackArgs = r9;
    Register temporary = rax;
    Register temporary2 = r11;
    Register opNumArgs = r10;
    Label aligned;
    Label pushThis;
    Label stackOverflow;

    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME_WITH_ARGV);

    __ Push(numArgs);
    __ Cmpq(0, numArgs);
    __ Jz(&pushThis);
    __ Movq(numArgs, opNumArgs);
    PushArgsWithArgvAndCheckStack(assembler, glue, opNumArgs, stackArgs, temporary, temporary2, &stackOverflow);

    __ Bind(&pushThis);
    __ Push(thisValue);
    // new.target
    if (callNew) {
        __ Pushq(func);
    } else {
        __ Pushq(JSTaggedValue::Undefined().GetRawData());
    }
    __ Pushq(func);
    // 40: skip frame type, numArgs, func, newTarget and this
    __ Leaq(Operand(rsp, numArgs, Times8, 5 * FRAME_SLOT_SIZE), rbp);
    __ Movq(rsp, stackArgs);

    // push argc
    __ Addl(NUM_MANDATORY_JSFUNC_ARGS, numArgs);
    __ Pushq(numArgs);
    // push thread
    __ Pushq(glue);
    // EcmaRuntimeCallInfo
    __ Movq(rsp, rdi);

    __ Testq(0xf, rsp);  // 0xf: 0x1111
    __ Jz(&aligned, Distance::Near);
    __ PushAlignBytes();

    __ Bind(&aligned);
    CallNativeInternal(assembler, nativeCode);
    __ Ret();

    __ Bind(&stackOverflow);
    {
        Label aligneThrow;
        __ Movq(static_cast<int32_t>(FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME),
            Operand(rsp, FRAME_SLOT_SIZE));
        __ Pushq(0);  // argc
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED);  // this
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED);  // newTarget
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED);  // callTarget

        __ Testq(0xf, rsp);  // 0xf: 0x1111
        __ Jz(&aligneThrow, Distance::Near);
        __ PushAlignBytes();

        __ Bind(&aligneThrow);
        Register trampolineIdRegister = r9;
        Register trampolineRegister = r10;
        __ Movq(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException, trampolineIdRegister);
        __ Movq(Operand(glue, trampolineIdRegister, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)),
            trampolineRegister);
        __ Callq(trampolineRegister);

        // resume rsp
        __ Movq(rbp, rsp);
        __ Pop(rbp);
        __ Ret();
    }
}

void AsmInterpreterCall::CallNativeEntry(ExtendedAssembler *assembler)
{
    Register glue = rdi;
    Register argv = r9;
    Register method = rdx;
    Register function = rsi;
    Register nativeCode = r10;

    __ PushAlignBytes();
    __ Push(function);
    // 24: skip thread & argc & returnAddr
    __ Subq(3 * FRAME_SLOT_SIZE, rsp);
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_ENTRY_FRAME);
    __ Movq(Operand(method, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET), nativeCode); // get native pointer
    __ Movq(argv, r11);
    // 16: skip numArgs & thread
    __ Subq(2 * FRAME_SLOT_SIZE, r11);
    // EcmaRuntimeCallInfo
    __ Movq(r11, rdi);

    CallNativeInternal(assembler, nativeCode);

    // 40: skip function
    __ Addq(5 * FRAME_SLOT_SIZE, rsp);
    __ Ret();
}

// uint64_t PushCallArgsAndDispatchNative(uintptr_t codeAddress, uintptr_t glue, uint32_t argc, ...)
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:        %rax - codeAddress
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
    Register nativeCode = rax;
    Register glue = rdi;

    __ Movq(Operand(rsp, FRAME_SLOT_SIZE), glue); // 8: glue
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME);
    __ Leaq(Operand(rbp, 2 * FRAME_SLOT_SIZE), rdi); // 16: skip argc & thread
    __ PushAlignBytes();
    CallNativeInternal(assembler, nativeCode);
    __ Ret();
}

void AsmInterpreterCall::PushBuiltinFrame(ExtendedAssembler *assembler,
                                          Register glue, FrameType type)
{
    __ Pushq(rbp);
    __ Movq(rsp, Operand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Pushq(static_cast<int32_t>(type));
    __ Leaq(Operand(rsp, FRAME_SLOT_SIZE), rbp);  // 8: skip frame type
}

void AsmInterpreterCall::CallNativeInternal(ExtendedAssembler *assembler, Register nativeCode)
{
    __ Callq(nativeCode);
    // resume rsp
    __ Movq(rbp, rsp);
    __ Pop(rbp);
}

// ResumeRspAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// %r13 - glue
// %rbp - sp
// %r12 - pc
// %rbx - constantPool
// %r14 - profileTypeInfo
// %rsi - acc
// %rdi - hotnessCounter
// %r8  - jumpSizeAfterCall
void AsmInterpreterCall::ResumeRspAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndDispatch));
    Register glueRegister = __ GlueRegister();
    Register spRegister = rbp;
    Register pcRegister = r12;
    Register ret = rsi;
    Register jumpSizeRegister = r8;

    Register frameStateBaseRegister = r11;
    __ Movq(spRegister, frameStateBaseRegister);
    __ Subq(AsmInterpretedFrame::GetSize(false), frameStateBaseRegister);

    Label dispatch;
    Label newObjectRangeReturn;
    __ Cmpq(0, jumpSizeRegister);
    __ Jle(&newObjectRangeReturn);

    __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetBaseOffset(false)), spRegister);  // update sp
    __ Addq(jumpSizeRegister, pcRegister);  // newPC
    Register temp = rax;
    Register opcodeRegister = rax;
    __ Movzbq(Operand(pcRegister, 0), opcodeRegister);

    __ Bind(&dispatch);
    {
        __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetFpOffset(false)), rsp);   // resume rsp
        Register bcStubRegister = r11;
        __ Movq(Operand(glueRegister, opcodeRegister, Times8, JSThread::GlueData::GetBCStubEntriesOffset(false)),
            bcStubRegister);
        __ Jmp(bcStubRegister);
    }

    Label getThis;
    Label notUndefined;
    __ Bind(&newObjectRangeReturn);
    __ Cmpq(JSTaggedValue::Undefined().GetRawData(), ret);
    __ Jne(&notUndefined);

    __ Bind(&getThis);
    __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetBaseOffset(false)), spRegister);  // update sp
    __ Subq(jumpSizeRegister, pcRegister);  // sub negative jmupSize
    __ Movzbq(Operand(pcRegister, 0), opcodeRegister);
    {
        __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetThisOffset(false)), ret);
        __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetFpOffset(false)), rsp);   // resume rsp
        Register bcStubRegister = r11;
        __ Movq(Operand(glueRegister, opcodeRegister, Times8, JSThread::GlueData::GetBCStubEntriesOffset(false)),
            bcStubRegister);
        __ Jmp(bcStubRegister);
    }

    __ Bind(&notUndefined);
    {
        Label notEcmaObject;
        __ Movabs(JSTaggedValue::TAG_HEAPOBJECT_MASK, temp);
        __ And(ret, temp);
        __ Cmpq(0, temp);
        __ Jne(&notEcmaObject);
        // acc is heap object
        __ Movq(Operand(ret, 0), temp);  // hclass
        __ Movl(Operand(temp, JSHClass::BIT_FIELD_OFFSET), temp);
        __ Cmpb(static_cast<int32_t>(JSType::ECMA_OBJECT_LAST), temp);
        __ Ja(&notEcmaObject);
        __ Cmpb(static_cast<int32_t>(JSType::ECMA_OBJECT_FIRST), temp);
        __ Jb(&notEcmaObject);
        // acc is ecma object
        __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetBaseOffset(false)), spRegister);  // update sp
        __ Subq(jumpSizeRegister, pcRegister);  // sub negative jmupSize
        __ Movzbq(Operand(pcRegister, 0), opcodeRegister);
        __ Jmp(&dispatch);

        __ Bind(&notEcmaObject);
        {
            // load constructor
            __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetFunctionOffset(false)), temp);
            __ Movq(Operand(temp, JSFunctionBase::METHOD_OFFSET), temp);
            __ Movq(Operand(temp, Method::EXTRA_LITERAL_INFO_OFFSET), temp);
            __ Shr(MethodLiteral::FunctionKindBits::START_BIT, temp);
            __ Andl((1LU << MethodLiteral::FunctionKindBits::SIZE) - 1, temp);
            __ Cmpl(static_cast<int32_t>(FunctionKind::CLASS_CONSTRUCTOR), temp);
            __ Jbe(&getThis);  // constructor is base
            // fall through
        }
        // exception branch
        {
            __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetBaseOffset(false)), spRegister);
            __ Movq(kungfu::BytecodeStubCSigns::ID_NewObjectRangeThrowException, opcodeRegister);
            __ Jmp(&dispatch);
        }
    }
}

// c++ calling convention
// %rdi - glue
// %rsi - callTarget
// %rdx - method
// %rcx - callField
// %r8 - receiver
// %r9 - value
void AsmInterpreterCall::CallGetter(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallGetter));
    Label target;

    PushAsmInterpBridgeFrame(assembler);
    __ Callq(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    JSCallCommonEntry(assembler, JSCallMode::CALL_GETTER);
}

void AsmInterpreterCall::CallSetter(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallSetter));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Callq(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    JSCallCommonEntry(assembler, JSCallMode::CALL_SETTER);
}

// Input: glue             - %rdi
//        callTarget       - %rsi
//        method           - %rdx
//        callField        - %rcx
//        arg0(argc)       - %r8
//        arg1(arglist)    - %r9
//        argthis          - stack
void AsmInterpreterCall::CallReturnWithArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallReturnWithArgv));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    Register r13 = __ CppJSCallAvailableRegister1();
    __ Movq(Operand(rbp, FRAME_SLOT_SIZE), r13);
    __ Callq(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
    }
}

void AsmInterpreterCall::CallContainersArgs3(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallContainersArgs3));
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    GetArgvAtStack(assembler);
    __ Callq(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
    }
}

// ResumeRspAndReturn(uintptr_t acc)
// GHC calling convention
// %r13 - acc
// %rbp - prevSp
// %r12 - sp
void AsmInterpreterCall::ResumeRspAndReturn(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndReturn));
    Register currentSp = r12;
    Register fpRegister = r10;
    intptr_t offset = AsmInterpretedFrame::GetFpOffset(false) - AsmInterpretedFrame::GetSize(false);
    __ Movq(Operand(currentSp, static_cast<int32_t>(offset)), fpRegister);
    __ Movq(fpRegister, rsp);
    // return
    {
        __ Movq(r13, rax);
        __ Ret();
    }
}

// ResumeCaughtFrameAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter)
// GHC calling convention
// %r13 - glue
// %rbp - sp
// %r12 - pc
// %rbx - constantPool
// %r14 - profileTypeInfo
// %rsi - acc
// %rdi - hotnessCounter
void AsmInterpreterCall::ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeCaughtFrameAndDispatch));
    Register glueRegister = __ GlueRegister();
    Register pcRegister = r12;

    Label dispatch;
    Register fpRegister = r11;
    __ Movq(Operand(glueRegister, JSThread::GlueData::GetLastFpOffset(false)), fpRegister);
    __ Cmpq(0, fpRegister);
    __ Jz(&dispatch);
    __ Movq(fpRegister, rsp);  // resume rsp
    __ Bind(&dispatch);
    {
        Register opcodeRegister = rax;
        __ Movzbq(Operand(pcRegister, 0), opcodeRegister);
        Register bcStubRegister = r11;
        __ Movq(Operand(glueRegister, opcodeRegister, Times8, JSThread::GlueData::GetBCStubEntriesOffset(false)),
            bcStubRegister);
        __ Jmp(bcStubRegister);
    }
}

// ResumeUncaughtFrameAndReturn(uintptr_t glue)
// GHC calling convention
// %r13 - glue
// %rbp - sp
// %r12 - acc
void AsmInterpreterCall::ResumeUncaughtFrameAndReturn(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeUncaughtFrameAndReturn));
    Register glueRegister = __ GlueRegister();
    Register acc(r12);
    Register cppRet(rax);

    Label ret;
    Register fpRegister = r11;
    __ Movq(Operand(glueRegister, JSThread::GlueData::GetLastFpOffset(false)), fpRegister);
    __ Cmpq(0, fpRegister);
    __ Jz(&ret);
    __ Movq(fpRegister, rsp);  // resume rsp
    __ Bind(&ret);
    // this method will return to Execute(cpp calling convention), and the return value should be put into rax.
    __ Movq(acc, cppRet);
    __ Ret();
}

// ResumeRspAndRollback(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// %r13 - glue
// %rbp - sp
// %r12 - pc
// %rbx - constantPool
// %r14 - profileTypeInfo
// %rsi - acc
// %rdi - hotnessCounter
// %r8  - jumpSizeAfterCall
void AsmInterpreterCall::ResumeRspAndRollback(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndRollback));
    Register glueRegister = __ GlueRegister();
    Register spRegister = rbp;
    Register pcRegister = r12;
    Register ret = rsi;
    Register jumpSizeRegister = r8;

    Register frameStateBaseRegister = r11;
    __ Movq(spRegister, frameStateBaseRegister);
    __ Subq(AsmInterpretedFrame::GetSize(false), frameStateBaseRegister);

    __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetBaseOffset(false)), spRegister);  // update sp
    __ Addq(jumpSizeRegister, pcRegister);  // newPC
    Register opcodeRegister = rax;
    __ Movzbq(Operand(pcRegister, 0), opcodeRegister);

    __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetFunctionOffset(false)), ret); // restore acc

    __ Movq(Operand(frameStateBaseRegister, AsmInterpretedFrame::GetFpOffset(false)), rsp);   // resume rsp
    Register bcStubRegister = r11;
    __ Movq(Operand(glueRegister, opcodeRegister, Times8, JSThread::GlueData::GetBCStubEntriesOffset(false)),
        bcStubRegister);
    __ Jmp(bcStubRegister);
}

void AsmInterpreterCall::PushUndefinedWithArgcAndCheckStack(ExtendedAssembler *assembler, Register glue, Register argc,
    Register op1, Register op2, Label *stackOverflow)
{
    ASSERT(stackOverflow != nullptr);
    StackOverflowCheck(assembler, glue, argc, op1, op2, stackOverflow);
    PushUndefinedWithArgc(assembler, argc);
}

void AsmInterpreterCall::ThrowStackOverflowExceptionAndReturn(ExtendedAssembler *assembler, Register glue, Register fp,
    Register op)
{
    if (fp != rsp) {
        __ Movq(fp, rsp);
    }
    __ Movq(kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException, op);
    __ Movq(Operand(glue, op, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), op);
    if (glue != r13) {
        __ Movq(glue, r13);
    }
    __ Callq(op);
    __ Ret();
}

void AsmInterpreterCall::HasPendingException([[maybe_unused]] ExtendedAssembler *assembler,
    [[maybe_unused]] Register threadRegister)
{
}
#undef __
}  // namespace panda::ecmascript::x64