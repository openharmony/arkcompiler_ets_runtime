/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/interpreter/interpreter_assembly.h"

#include "ecmascript/ic/ic_runtime_stub-inl.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/base/gc_helper.h"

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif

namespace panda::ecmascript {
using panda::ecmascript::kungfu::CommonStubCSigns;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-ptr-dereference"
#pragma clang diagnostic ignored "-Wgnu-label-as-value"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if ECMASCRIPT_ENABLE_INTERPRETER_LOG
#define LOG_INST() LOG_INTERPRETER(DEBUG)
#else
#define LOG_INST() false && LOG_INTERPRETER(DEBUG)
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ADVANCE_PC(offset) \
    pc += (offset);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic, cppcoreguidelines-macro-usage)

#define GOTO_NEXT()  // NOLINT(clang-diagnostic-gnu-label-as-value, cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISPATCH_OFFSET(offset)                                                                               \
    do {                                                                                                      \
        ADVANCE_PC(offset)                                                                                    \
        SAVE_PC();                                                                                            \
        SAVE_ACC();                                                                                           \
        AsmInterpretedFrame *frame = GET_ASM_FRAME(sp);                                                       \
        auto currentMethod = ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget();            \
        currentMethod->SetHotnessCounter(static_cast<int16_t>(hotnessCounter));                               \
        return;                                                                                               \
    } while (false)

#define DISPATCH(opcode)  DISPATCH_OFFSET(BytecodeInstruction::Size(BytecodeInstruction::Opcode::opcode))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_ASM_FRAME(CurrentSp) \
    (reinterpret_cast<AsmInterpretedFrame *>(CurrentSp) - 1) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#define GET_ENTRY_FRAME(sp) \
    (reinterpret_cast<InterpretedEntryFrame *>(sp) - 1)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#define GET_BUILTIN_FRAME(sp) \
    (reinterpret_cast<InterpretedBuiltinFrame *>(sp) - 1)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SAVE_PC() (GET_ASM_FRAME(sp)->pc = pc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SAVE_ACC() (GET_ASM_FRAME(sp)->acc = acc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RESTORE_ACC() (acc = GET_ASM_FRAME(sp)->acc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_GOTO_EXCEPTION_HANDLER()                                                                        \
    do {                                                                                                            \
        SAVE_PC();                                                                                                  \
        GET_ASM_FRAME(sp)->acc = JSTaggedValue::Exception();                                                        \
        return;                                                                                                     \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_HANDLE_RETURN()                                                     \
    do {                                                                                \
        JSFunction* prevFunc = JSFunction::Cast(prevState->function.GetTaggedObject()); \
        method = prevFunc->GetCallTarget();                                             \
        hotnessCounter = static_cast<int32_t>(method->GetHotnessCounter());             \
        ASSERT(prevState->callSize == GetJumpSizeAfterCall(pc));                        \
        DISPATCH_OFFSET(prevState->callSize);                                           \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_RETURN_IF_ABRUPT(result)      \
    do {                                          \
        if ((result).IsException()) {             \
            INTERPRETER_GOTO_EXCEPTION_HANDLER(); \
        }                                         \
    } while (false)

#if ECMASCRIPT_ENABLE_IC
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UPDATE_HOTNESS_COUNTER(offset)                                      \
    do {                                                                    \
        hotnessCounter += offset;                                           \
        if (UNLIKELY(hotnessCounter <= 0)) {                                \
            SAVE_ACC();                                                     \
            profileTypeInfo = UpdateHotnessCounter(thread, sp);             \
            RESTORE_ACC();                                                  \
            hotnessCounter = EcmaInterpreter::METHOD_HOTNESS_THRESHOLD;     \
        }                                                                   \
    } while (false)
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UPDATE_HOTNESS_COUNTER(offset) static_cast<void>(0)
#endif

#define READ_INST_OP() READ_INST_8(0)               // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_0() (READ_INST_8(1) & 0xf)      // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_1() (READ_INST_8(1) >> 4 & 0xf) // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_2() (READ_INST_8(2) & 0xf)      // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_3() (READ_INST_8(2) >> 4 & 0xf) // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_0() READ_INST_8(1)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_1() READ_INST_8(2)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_2() READ_INST_8(3)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_3() READ_INST_8(4)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_4() READ_INST_8(5)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_5() READ_INST_8(6)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_6() READ_INST_8(7)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_7() READ_INST_8(8)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_8() READ_INST_8(9)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_9() READ_INST_8(10)             // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8(offset) (*(pc + (offset)))
#define MOVE_AND_READ_INST_8(currentInst, offset) \
    (currentInst) <<= 8;                          \
    (currentInst) += READ_INST_8(offset);         \

#define READ_INST_16_0() READ_INST_16(2)
#define READ_INST_16_1() READ_INST_16(3)
#define READ_INST_16_2() READ_INST_16(4)
#define READ_INST_16_3() READ_INST_16(5)
#define READ_INST_16_5() READ_INST_16(7)
#define READ_INST_16_7() READ_INST_16(9)
#define READ_INST_16(offset)                            \
    ({                                                  \
        uint16_t currentInst = READ_INST_8(offset);     \
        MOVE_AND_READ_INST_8(currentInst, (offset) - 1) \
    })

#define READ_INST_32_0() READ_INST_32(4)
#define READ_INST_32_1() READ_INST_32(5)
#define READ_INST_32_2() READ_INST_32(6)
#define READ_INST_32(offset)                            \
    ({                                                  \
        uint32_t currentInst = READ_INST_8(offset);     \
        MOVE_AND_READ_INST_8(currentInst, (offset) - 1) \
        MOVE_AND_READ_INST_8(currentInst, (offset) - 2) \
        MOVE_AND_READ_INST_8(currentInst, (offset) - 3) \
    })

#define READ_INST_64_0()                       \
    ({                                         \
        uint64_t currentInst = READ_INST_8(8); \
        MOVE_AND_READ_INST_8(currentInst, 7)   \
        MOVE_AND_READ_INST_8(currentInst, 6)   \
        MOVE_AND_READ_INST_8(currentInst, 5)   \
        MOVE_AND_READ_INST_8(currentInst, 4)   \
        MOVE_AND_READ_INST_8(currentInst, 3)   \
        MOVE_AND_READ_INST_8(currentInst, 2)   \
        MOVE_AND_READ_INST_8(currentInst, 1)   \
    })

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_VREG(idx) (sp[idx])  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_VREG_VALUE(idx) (JSTaggedValue(sp[idx]))  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_VREG(idx, val) (sp[idx] = (val));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#define GET_ACC() (acc)                        // NOLINT(cppcoreguidelines-macro-usage)
#define SET_ACC(val) (acc = val)               // NOLINT(cppcoreguidelines-macro-usage)

using InterpreterEntry = JSTaggedType (*)(uintptr_t glue, ECMAObject *callTarget,
    Method *method, uint64_t callField, size_t argc, uintptr_t argv);
using GeneratorReEnterInterpEntry = JSTaggedType (*)(uintptr_t glue, JSTaggedType context);

void InterpreterAssembly::InitStackFrame(JSThread *thread)
{
    InitStackFrameForSP(const_cast<JSTaggedType *>(thread->GetCurrentSPFrame()));
}

void InterpreterAssembly::InitStackFrameForSP(JSTaggedType *prevSp)
{
    InterpretedEntryFrame *entryState = InterpretedEntryFrame::GetFrameFromSp(prevSp);
    entryState->base.type = FrameType::INTERPRETER_ENTRY_FRAME;
    entryState->base.prev = nullptr;
    entryState->pc = nullptr;
}

JSTaggedValue InterpreterAssembly::Execute(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    INTERPRETER_TRACE(thread, AsmExecute);
#if ECMASCRIPT_ENABLE_INTERPRETER_ARKUINAITVE_TRACE
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_MAX, HITRACE_TAG_ARK, "ArkCompiler::InterpreterAssembly::Execute", "");
#endif
    // When the  function is jit-compiled, the Method object is reinstalled.
    // In this case, the AotWithCall field may be updated.
    // This causes a Construct that is not a ClassConstructor to call jit code.
    ECMAObject *callTarget = reinterpret_cast<ECMAObject*>(info->GetFunctionValue().GetTaggedObject());
    Method *method = callTarget->GetCallTarget();
    bool isCompiledCode = JSFunctionBase::IsCompiledCodeFromCallTarget(info->GetFunctionValue());

    // check is or not debugger
    thread->CheckSwitchDebuggerBCStub();
    thread->CheckSafepoint();
    uint32_t argc = info->GetArgsNumber();
    uintptr_t argv = reinterpret_cast<uintptr_t>(info->GetArgs());

    callTarget = reinterpret_cast<ECMAObject*>(info->GetFunctionValue().GetTaggedObject());
    method = callTarget->GetCallTarget();
    if (isCompiledCode) {
        JSHandle<JSFunction> func(thread, info->GetFunctionValue());
        if (func->IsClassConstructor()) {
            {
                EcmaVM *ecmaVm = thread->GetEcmaVM();
                ObjectFactory *factory = ecmaVm->GetFactory();
                JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                    "class constructor cannot called without 'new'", StackCheck::NO);
                thread->SetException(error.GetTaggedValue());
            }
            return thread->GetException();
        }
        JSTaggedValue res = JSFunction::InvokeOptimizedEntrypoint(thread, func, info);
        const JSTaggedType *curSp = thread->GetCurrentSPFrame();
        InterpretedEntryFrame *entryState = InterpretedEntryFrame::GetFrameFromSp(curSp);
        JSTaggedType *prevSp = entryState->base.prev;
        thread->SetCurrentSPFrame(prevSp);
        if (thread->HasPendingException()) {
            return thread->GetException();
        }
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
        thread->CheckJSTaggedType(JSTaggedValue(res).GetRawData());
#endif
        return JSTaggedValue(res);
    }
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    RuntimeStubs::StartCallTimer(thread->GetGlueAddr(), info->GetFunctionValue().GetRawData(), false);
#endif
    if (thread->IsDebugMode() && !method->IsNativeWithCallField()) {
        JSHandle<JSFunction> func(thread, info->GetFunctionValue());
        JSTaggedValue env = func->GetLexicalEnv();
        MethodEntry(thread, method, env);
    }
    if (thread->NeedReadBarrier()) {
        base::GCHelper::CopyCallTarget(callTarget); // callTarget should be ToSpace Reference
        method = callTarget->GetCallTarget();
    }
    // When C++ enters ASM, save the current globalenv and restore to glue after call
    SaveEnv envScope(thread);
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_AsmInterpreterEntry);
    auto acc = reinterpret_cast<InterpreterEntry>(entry)(thread->GetGlueAddr(),
        callTarget, method, method->GetCallField(), argc, argv);

    if (thread->IsEntryFrameDroppedTrue()) {
        thread->PendingEntryFrameDroppedState();
        return JSTaggedValue::Hole();
    }

    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    ASSERT(FrameHandler::GetFrameType(sp) == FrameType::INTERPRETER_ENTRY_FRAME);
    auto prevEntry = InterpretedEntryFrame::GetFrameFromSp(sp)->GetPrevFrameFp();
    thread->SetCurrentSPFrame(prevEntry);

#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
    thread->CheckJSTaggedType(JSTaggedValue(acc).GetRawData());
#endif
    return JSTaggedValue(acc);
}

void InterpreterAssembly::MethodEntry(JSThread *thread, Method *method, JSTaggedValue env)
{
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame()) {
            continue;
        }
        auto *debuggerMgr = thread->GetEcmaVM()->GetJsDebuggerManager();
        debuggerMgr->GetNotificationManager()->MethodEntryEvent(thread, method, env);
        return;
    }
}

int64_t InterpreterAssembly::GetCallSize(EcmaOpcode opcode)
{
    int64_t callSize = static_cast<int64_t>(BytecodeInstruction::Size(opcode));
    switch (opcode) {
        case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
            return -callSize;
        default:
            return callSize;
    }
    return callSize;
}

JSTaggedValue InterpreterAssembly::GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context)
{
    // check is or not debugger
    thread->CheckSwitchDebuggerBCStub();
    // When C++ enters ASM, save the current globalenv and restore to glue after call
    SaveEnv envScope(thread);
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_GeneratorReEnterAsmInterp);
    JSTaggedValue func = context->GetMethod();
    Method *method = ECMAObject::Cast(func.GetTaggedObject())->GetCallTarget();
    JSTaggedValue env = context->GetLexicalEnv();
    if (thread->IsDebugMode() && !method->IsNativeWithCallField()) {
        MethodEntry(thread, method, env);
    }
    if (thread->NeedReadBarrier()) {
        // func should be ToSpace Reference
        base::GCHelper::CopyCallTarget(func.GetTaggedObject());
        // context should be ToSpace Reference
        base::GCHelper::CopyGeneratorContext(context.GetObject<GeneratorContext>());
    }
    auto acc = reinterpret_cast<GeneratorReEnterInterpEntry>(entry)(thread->GetGlueAddr(), context.GetTaggedType());
    return JSTaggedValue(acc);
}
#ifndef EXCLUDE_C_INTERPRETER
void InterpreterAssembly::HandleMovV4V4(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t vdst = READ_INST_4_0();
    uint16_t vsrc = READ_INST_4_1();
    LOG_INST() << "mov v" << vdst << ", v" << vsrc;
    uint64_t value = GET_VREG(vsrc);
    SET_VREG(vdst, value)
    DISPATCH(MOV_V4_V4);
}

void InterpreterAssembly::HandleMovV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t vdst = READ_INST_8_0();
    uint16_t vsrc = READ_INST_8_1();
    LOG_INST() << "mov v" << vdst << ", v" << vsrc;
    uint64_t value = GET_VREG(vsrc);
    SET_VREG(vdst, value)
    DISPATCH(MOV_V8_V8);
}

void InterpreterAssembly::HandleMovV16V16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t vdst = READ_INST_16_0();
    uint16_t vsrc = READ_INST_16_2();
    LOG_INST() << "mov v" << vdst << ", v" << vsrc;
    uint64_t value = GET_VREG(vsrc);
    SET_VREG(vdst, value)
    DISPATCH(MOV_V16_V16);
}

void InterpreterAssembly::HandleLdaStrId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    LOG_INST() << "lda.str " << std::hex << stringId;
    constpool = GetConstantPool(sp);
    SET_ACC(ConstantPool::GetStringFromCache(thread, constpool, stringId));
    DISPATCH(LDA_STR_ID16);
}

void InterpreterAssembly::HandleJmpImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int8_t offset = READ_INST_8_0();
    UPDATE_HOTNESS_COUNTER(offset);
    LOG_INST() << "jmp " << std::hex << static_cast<int32_t>(offset);
    DISPATCH_OFFSET(offset);
}

void InterpreterAssembly::HandleJmpImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int16_t offset = static_cast<int16_t>(READ_INST_16_0());
    UPDATE_HOTNESS_COUNTER(offset);
    LOG_INST() << "jmp " << std::hex << static_cast<int32_t>(offset);
    DISPATCH_OFFSET(offset);
}

void InterpreterAssembly::HandleJmpImm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t offset = static_cast<int32_t>(READ_INST_32_0());
    UPDATE_HOTNESS_COUNTER(offset);
    LOG_INST() << "jmp " << std::hex << offset;
    DISPATCH_OFFSET(offset);
}

void InterpreterAssembly::HandleJeqzImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int8_t offset = READ_INST_8_0();
    LOG_INST() << "jeqz ->\t"
                << "cond jmpz " << std::hex << static_cast<int32_t>(offset);
    if (GET_ACC().IsFalse() || (GET_ACC().IsInt() && GET_ACC().GetInt() == 0) ||
        (GET_ACC().IsDouble() && GET_ACC().GetDouble() == 0)) {
        UPDATE_HOTNESS_COUNTER(offset);
        DISPATCH_OFFSET(offset);
    } else {
        DISPATCH(JEQZ_IMM8);
    }
}

void InterpreterAssembly::HandleJeqzImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int16_t offset = static_cast<int16_t>(READ_INST_16_0());
    LOG_INST() << "jeqz ->\t"
                << "cond jmpz " << std::hex << static_cast<int32_t>(offset);
    if (GET_ACC().IsFalse() || (GET_ACC().IsInt() && GET_ACC().GetInt() == 0) ||
        (GET_ACC().IsDouble() && GET_ACC().GetDouble() == 0)) {
        UPDATE_HOTNESS_COUNTER(offset);
        DISPATCH_OFFSET(offset);
    } else {
        DISPATCH(JEQZ_IMM16);
    }
}

void InterpreterAssembly::HandleJeqzImm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t offset = static_cast<int32_t>(READ_INST_32_0());
    LOG_INST() << "jeqz ->\t"
                << "cond jmpz " << std::hex << offset;
    if (GET_ACC().IsFalse() || (GET_ACC().IsInt() && GET_ACC().GetInt() == 0) ||
        (GET_ACC().IsDouble() && GET_ACC().GetDouble() == 0)) {
        UPDATE_HOTNESS_COUNTER(offset);
        DISPATCH_OFFSET(offset);
    } else {
        DISPATCH(JEQZ_IMM16);
    }
}

void InterpreterAssembly::HandleJnezImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int8_t offset = READ_INST_8_0();
    LOG_INST() << "jnez ->\t"
                << "cond jmpz " << std::hex << static_cast<int32_t>(offset);
    if (GET_ACC().IsTrue() || (GET_ACC().IsInt() && GET_ACC().GetInt() != 0) ||
        (GET_ACC().IsDouble() && GET_ACC().GetDouble() != 0)) {
        UPDATE_HOTNESS_COUNTER(offset);
        DISPATCH_OFFSET(offset);
    } else {
        DISPATCH(JNEZ_IMM8);
    }
}

void InterpreterAssembly::HandleJnezImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int16_t offset = static_cast<int16_t>(READ_INST_16_0());
    LOG_INST() << "jnez ->\t"
                << "cond jmpz " << std::hex << static_cast<int32_t>(offset);
    if (GET_ACC().IsTrue() || (GET_ACC().IsInt() && GET_ACC().GetInt() != 0) ||
        (GET_ACC().IsDouble() && GET_ACC().GetDouble() != 0)) {
        UPDATE_HOTNESS_COUNTER(offset);
        DISPATCH_OFFSET(offset);
    } else {
        DISPATCH(JNEZ_IMM16);
    }
}

void InterpreterAssembly::HandleJnezImm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t offset = static_cast<int32_t>(READ_INST_32_0());
    LOG_INST() << "jnez ->\t"
                << "cond jmpz " << std::hex << offset;
    if (GET_ACC().IsTrue() || (GET_ACC().IsInt() && GET_ACC().GetInt() != 0) ||
        (GET_ACC().IsDouble() && GET_ACC().GetDouble() != 0)) {
        UPDATE_HOTNESS_COUNTER(offset);
        DISPATCH_OFFSET(offset);
    } else {
        DISPATCH(JNEZ_IMM32);
    }
}

void InterpreterAssembly::HandleLdaV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t vsrc = READ_INST_8_0();
    LOG_INST() << "lda v" << vsrc;
    uint64_t value = GET_VREG(vsrc);
    SET_ACC(JSTaggedValue(value));
    DISPATCH(LDA_V8);
}

void InterpreterAssembly::HandleStaV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t vdst = READ_INST_8_0();
    LOG_INST() << "sta v" << vdst;
    SET_VREG(vdst, GET_ACC().GetRawData())
    DISPATCH(STA_V8);
}

void InterpreterAssembly::HandleLdaiImm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t imm = static_cast<int32_t>(READ_INST_32_0());
    LOG_INST() << "ldai " << std::hex << imm;
    SET_ACC(JSTaggedValue(imm));
    DISPATCH(LDAI_IMM32);
}

void InterpreterAssembly::HandleFldaiImm64(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    auto imm = base::bit_cast<double>(READ_INST_64_0());
    LOG_INST() << "fldai " << imm;
    SET_ACC(JSTaggedValue(imm));
    DISPATCH(FLDAI_IMM64);
}

void InterpreterAssembly::HandleReturn(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "returnla ";
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    LOG_INST() << "Exit: Runtime Call " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                            << std::hex << reinterpret_cast<uintptr_t>(state->pc);
    Method *method = ECMAObject::Cast(state->function.GetTaggedObject())->GetCallTarget();
    [[maybe_unused]] auto fistPC = method->GetBytecodeArray();
    UPDATE_HOTNESS_COUNTER(-(pc - fistPC));
    method->SetHotnessCounter(static_cast<int16_t>(hotnessCounter));
    sp = state->base.prev;
    ASSERT(sp != nullptr);

    AsmInterpretedFrame *prevState = GET_ASM_FRAME(sp);
    pc = prevState->pc;
    thread->SetCurrentFrame(sp);
    // entry frame
    if (pc == nullptr) {
        state->acc = acc;
        return;
    }

    // new stackless not supported
    INTERPRETER_HANDLE_RETURN();
}

void InterpreterAssembly::HandleReturnundefined(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "return.undefined";
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    LOG_INST() << "Exit: Runtime Call " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                            << std::hex << reinterpret_cast<uintptr_t>(state->pc);
    Method *method = ECMAObject::Cast(state->function.GetTaggedObject())->GetCallTarget();
    [[maybe_unused]] auto fistPC = method->GetBytecodeArray();
    UPDATE_HOTNESS_COUNTER(-(pc - fistPC));
    method->SetHotnessCounter(static_cast<int16_t>(hotnessCounter));
    sp = state->base.prev;
    ASSERT(sp != nullptr);

    AsmInterpretedFrame *prevState = GET_ASM_FRAME(sp);
    pc = prevState->pc;
    thread->SetCurrentFrame(sp);
    // entry frame
    if (pc == nullptr) {
        state->acc = JSTaggedValue::Undefined();
        return;
    }

    // new stackless not supported
    SET_ACC(JSTaggedValue::Undefined());
    INTERPRETER_HANDLE_RETURN();
}

void InterpreterAssembly::HandleLdnan(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldnan";
    SET_ACC(JSTaggedValue(base::NAN_VALUE));
    DISPATCH(LDNAN);
}

void InterpreterAssembly::HandleLdinfinity(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldinfinity";
    SET_ACC(JSTaggedValue(base::POSITIVE_INFINITY));
    DISPATCH(LDINFINITY);
}

void InterpreterAssembly::HandleLdundefined(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldundefined";
    SET_ACC(JSTaggedValue::Undefined());
    DISPATCH(LDUNDEFINED);
}

void InterpreterAssembly::HandleLdnull(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldnull";
    SET_ACC(JSTaggedValue::Null());
    DISPATCH(LDNULL);
}

void InterpreterAssembly::HandleLdsymbol(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldsymbol";
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    SET_ACC(globalEnv->GetSymbolFunction().GetTaggedValue());
    DISPATCH(LDSYMBOL);
}

void InterpreterAssembly::HandleLdglobal(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldglobal";
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    SET_ACC(globalObj);
    DISPATCH(LDGLOBAL);
}

void InterpreterAssembly::HandleLdtrue(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldtrue";
    SET_ACC(JSTaggedValue::True());
    DISPATCH(LDTRUE);
}

void InterpreterAssembly::HandleLdfalse(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldfalse";
    SET_ACC(JSTaggedValue::False());
    DISPATCH(LDFALSE);
}

void InterpreterAssembly::HandleGetunmappedargs(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::getunmappedargs";

    uint32_t startIdx = 0;
    uint32_t actualNumArgs = GetNumArgs(sp, 0, startIdx);

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetUnmapedArgs(thread, sp, actualNumArgs, startIdx);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETUNMAPPEDARGS);
}

void InterpreterAssembly::HandleAsyncfunctionenter(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::asyncfunctionenter";
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncFunctionEnter(thread);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(ASYNCFUNCTIONENTER);
}

void InterpreterAssembly::HandleTonumberImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::tonumber";
    JSTaggedValue value = GET_ACC();
    if (value.IsNumber()) {
        // fast path
        SET_ACC(value);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::ToNumber(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(TONUMBER_IMM8);
}

void InterpreterAssembly::HandleNegImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::neg";
    JSTaggedValue value = GET_ACC();
    // fast path
    if (value.IsInt()) {
        if (value.GetInt() == 0) {
            SET_ACC(JSTaggedValue(-0.0));
        } else {
            SET_ACC(JSTaggedValue(-value.GetInt()));
        }
    } else if (value.IsDouble()) {
        SET_ACC(JSTaggedValue(-value.GetDouble()));
    } else {  // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Neg(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(NEG_IMM8);
}

void InterpreterAssembly::HandleNotImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::not";
    JSTaggedValue value = GET_ACC();
    int32_t number;
    // number, fast path
    if (value.IsInt()) {
        number = static_cast<int32_t>(value.GetInt());
        SET_ACC(JSTaggedValue(~number));  // NOLINT(hicpp-signed-bitwise);
    } else if (value.IsDouble()) {
        number = base::NumberHelper::DoubleToInt(value.GetDouble(), base::INT32_BITS);
        SET_ACC(JSTaggedValue(~number));  // NOLINT(hicpp-signed-bitwise);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Not(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(NOT_IMM8);
}

void InterpreterAssembly::HandleIncImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::inc";
    JSTaggedValue value = GET_ACC();
    // number fast path
    if (value.IsInt()) {
        int32_t a0 = value.GetInt();
        if (UNLIKELY(a0 == INT32_MAX)) {
            auto ret = static_cast<double>(a0) + 1.0;
            SET_ACC(JSTaggedValue(ret));
        } else {
            SET_ACC(JSTaggedValue(a0 + 1));
        }
    } else if (value.IsDouble()) {
        SET_ACC(JSTaggedValue(value.GetDouble() + 1.0));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Inc(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(INC_IMM8);
}

void InterpreterAssembly::HandleDecImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::dec";
    JSTaggedValue value = GET_ACC();
    // number, fast path
    if (value.IsInt()) {
        int32_t a0 = value.GetInt();
        if (UNLIKELY(a0 == INT32_MIN)) {
            auto ret = static_cast<double>(a0) - 1.0;
            SET_ACC(JSTaggedValue(ret));
        } else {
            SET_ACC(JSTaggedValue(a0 - 1));
        }
    } else if (value.IsDouble()) {
        SET_ACC(JSTaggedValue(value.GetDouble() - 1.0));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Dec(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(DEC_IMM8);
}

void InterpreterAssembly::HandleThrow(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::throw";
    SAVE_PC();
    SlowRuntimeStub::Throw(thread, GET_ACC());
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleTypeofImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::typeof";
    JSTaggedValue res = FastRuntimeStub::FastTypeOf(thread, GET_ACC());
    SET_ACC(res);
    DISPATCH(TYPEOF_IMM8);
}

void InterpreterAssembly::HandleGetpropiterator(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::getpropiterator";
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetPropIterator(thread, GET_ACC());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETPROPITERATOR);
}

void InterpreterAssembly::HandleResumegenerator(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::resumegenerator";
    JSTaggedValue objVal = GET_ACC();
    if (objVal.IsAsyncGeneratorObject()) {
        JSAsyncGeneratorObject *obj = JSAsyncGeneratorObject::Cast(objVal.GetTaggedObject());
        SET_ACC(obj->GetResumeResult());
    } else {
        JSGeneratorObject *obj = JSGeneratorObject::Cast(objVal.GetTaggedObject());
        SET_ACC(obj->GetResumeResult());
    }
    DISPATCH(RESUMEGENERATOR);
}

void InterpreterAssembly::HandleGetresumemode(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::getresumemode";
    JSTaggedValue objVal = GET_ACC();
    if (objVal.IsAsyncGeneratorObject()) {
        JSAsyncGeneratorObject *obj = JSAsyncGeneratorObject::Cast(objVal.GetTaggedObject());
        SET_ACC(JSTaggedValue(static_cast<int>(obj->GetResumeMode())));
    } else {
        JSGeneratorObject *obj = JSGeneratorObject::Cast(objVal.GetTaggedObject());
        SET_ACC(JSTaggedValue(static_cast<int>(obj->GetResumeMode())));
    }
    DISPATCH(GETRESUMEMODE);
}

void InterpreterAssembly::HandleGetiteratorImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::getiterator";
    JSTaggedValue obj = GET_ACC();
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetIterator(thread, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETITERATOR_IMM8);
}

void InterpreterAssembly::HandleGetasynciteratorImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::getasynciterator";
    JSTaggedValue obj = GET_ACC();
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetAsyncIterator(thread, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETASYNCITERATOR_IMM8);
}

void InterpreterAssembly::HandleThrowConstassignmentPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "throwconstassignment"
                << " v" << v0;
    SAVE_PC();
    SlowRuntimeStub::ThrowConstAssignment(thread, GET_VREG_VALUE(v0));
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleThrowPatternnoncoerciblePrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "throwpatternnoncoercible";

    SAVE_PC();
    SlowRuntimeStub::ThrowPatternNonCoercible(thread);
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleThrowIfnotobjectPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "throwifnotobject";
    uint16_t v0 = READ_INST_8_1();

    JSTaggedValue value = GET_VREG_VALUE(v0);
    // fast path
    if (value.IsECMAObject()) {
        DISPATCH(THROW_IFNOTOBJECT_PREF_V8);
    }

    // slow path
    SAVE_PC();
    SlowRuntimeStub::ThrowIfNotObject(thread);
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleCloseiteratorImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::closeiterator"
               << " v" << v0;
    SAVE_PC();
    JSTaggedValue iter = GET_VREG_VALUE(v0);
    JSTaggedValue res = SlowRuntimeStub::CloseIterator(thread, iter);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CLOSEITERATOR_IMM8_V8);
}

void InterpreterAssembly::HandleAdd2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::add2"
               << " v" << v0;
    int32_t a0;
    int32_t a1;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // number, fast path
    if (left.IsInt() && right.IsInt()) {
        a0 = left.GetInt();
        a1 = right.GetInt();
        if ((a0 > 0 && a1 > INT32_MAX - a0) || (a0 < 0 && a1 < INT32_MIN - a0)) {
            auto ret = static_cast<double>(a0) + static_cast<double>(a1);
            SET_ACC(JSTaggedValue(ret));
        } else {
            SET_ACC(JSTaggedValue(a0 + a1));
        }
    } else if (left.IsNumber() && right.IsNumber()) {
        double a0Double = left.IsInt() ? left.GetInt() : left.GetDouble();
        double a1Double = right.IsInt() ? right.GetInt() : right.GetDouble();
        double ret = a0Double + a1Double;
        SET_ACC(JSTaggedValue(ret));
    } else {
        // one or both are not number, slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Add2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(ADD2_IMM8_V8);
}

void InterpreterAssembly::HandleSub2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::sub2"
               << " v" << v0;
    int32_t a0;
    int32_t a1;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    if (left.IsInt() && right.IsInt()) {
        a0 = left.GetInt();
        a1 = -right.GetInt();
        if ((a0 > 0 && a1 > INT32_MAX - a0) || (a0 < 0 && a1 < INT32_MIN - a0)) {
            auto ret = static_cast<double>(a0) + static_cast<double>(a1);
            SET_ACC(JSTaggedValue(ret));
        } else {
            SET_ACC(JSTaggedValue(a0 + a1));
        }
    } else if (left.IsNumber() && right.IsNumber()) {
        double a0Double = left.IsInt() ? left.GetInt() : left.GetDouble();
        double a1Double = right.IsInt() ? right.GetInt() : right.GetDouble();
        double ret = a0Double - a1Double;
        SET_ACC(JSTaggedValue(ret));
    } else {
        // one or both are not number, slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Sub2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(SUB2_IMM8_V8);
}
void InterpreterAssembly::HandleMul2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::mul2"
                << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = acc;
    JSTaggedValue value = FastRuntimeStub::FastMul(left, right);
    if (!value.IsHole()) {
        SET_ACC(value);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Mul2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(MUL2_IMM8_V8);
}

void InterpreterAssembly::HandleDiv2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::div2"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = acc;
    // fast path
    JSTaggedValue res = FastRuntimeStub::FastDiv(left, right);
    if (!res.IsHole()) {
        SET_ACC(res);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue slowRes = SlowRuntimeStub::Div2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(slowRes);
        SET_ACC(slowRes);
    }
    DISPATCH(DIV2_IMM8_V8);
}

void InterpreterAssembly::HandleMod2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t vs = READ_INST_8_1();
    LOG_INST() << "intrinsics::mod2"
                << " v" << vs;
    JSTaggedValue left = GET_VREG_VALUE(vs);
    JSTaggedValue right = GET_ACC();

    JSTaggedValue res = FastRuntimeStub::FastMod(left, right);
    if (!res.IsHole()) {
        SET_ACC(res);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue slowRes = SlowRuntimeStub::Mod2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(slowRes);
        SET_ACC(slowRes);
    }
    DISPATCH(MOD2_IMM8_V8);
}

void InterpreterAssembly::HandleEqImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::eq"
                << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = acc;
    JSTaggedValue res = FastRuntimeStub::FastEqual(left, right);
    if (!res.IsHole()) {
        SET_ACC(res);
    } else {
        // slow path
        SAVE_PC();
        res = SlowRuntimeStub::Eq(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }

    DISPATCH(EQ_IMM8_V8);
}

void InterpreterAssembly::HandleNoteqImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::noteq"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = acc;

    JSTaggedValue res = FastRuntimeStub::FastEqual(left, right);
    if (!res.IsHole()) {
        res = res.IsTrue() ? JSTaggedValue::False() : JSTaggedValue::True();
        SET_ACC(res);
    } else {
        // slow path
        SAVE_PC();
        res = SlowRuntimeStub::NotEq(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(NOTEQ_IMM8_V8);
}

void InterpreterAssembly::HandleLessImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::less"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    if (left.IsInt() && right.IsInt()) {
        // fast path
        bool ret = left.GetInt() < right.GetInt();
        SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False());
    } else if (left.IsNumber() && right.IsNumber()) {
        // fast path
        double valueA = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
        double valueB = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
        bool ret = JSTaggedValue::StrictNumberCompare(valueA, valueB) == ComparisonResult::LESS;
        SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False());
    } else if (left.IsBigInt() && right.IsBigInt()) {
        bool result = BigInt::LessThan(left, right);
        SET_ACC(JSTaggedValue(result));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Less(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(LESS_IMM8_V8);
}

void InterpreterAssembly::HandleLesseqImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t vs = READ_INST_8_1();
    LOG_INST() << "intrinsics::lesseq "
               << " v" << vs;
    JSTaggedValue left = GET_VREG_VALUE(vs);
    JSTaggedValue right = GET_ACC();
    if (left.IsInt() && right.IsInt()) {
        // fast path
        bool ret = ((left.GetInt() < right.GetInt()) || (left.GetInt() == right.GetInt()));
        SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False());
    } else if (left.IsNumber() && right.IsNumber()) {
        // fast path
        double valueA = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
        double valueB = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
        bool ret = JSTaggedValue::StrictNumberCompare(valueA, valueB) <= ComparisonResult::EQUAL;
        SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False());
    } else if (left.IsBigInt() && right.IsBigInt()) {
        bool result = BigInt::LessThan(left, right) || BigInt::Equal(left, right);
        SET_ACC(JSTaggedValue(result));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::LessEq(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(LESSEQ_IMM8_V8);
}

void InterpreterAssembly::HandleGreaterImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::greater"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = acc;
    if (left.IsInt() && right.IsInt()) {
        // fast path
        bool ret = left.GetInt() > right.GetInt();
        SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False());
    } else if (left.IsNumber() && right.IsNumber()) {
        // fast path
        double valueA = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
        double valueB = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
        bool ret = JSTaggedValue::StrictNumberCompare(valueA, valueB) == ComparisonResult::GREAT;
        SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False());
    } else if (left.IsBigInt() && right.IsBigInt()) {
        bool result = BigInt::LessThan(right, left);
        SET_ACC(JSTaggedValue(result));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Greater(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(GREATER_IMM8_V8);
}

void InterpreterAssembly::HandleGreatereqImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t vs = READ_INST_8_1();
    LOG_INST() << "intrinsics::greateq "
               << " v" << vs;
    JSTaggedValue left = GET_VREG_VALUE(vs);
    JSTaggedValue right = GET_ACC();
    if (left.IsInt() && right.IsInt()) {
        // fast path
        bool ret = ((left.GetInt() > right.GetInt()) || (left.GetInt() == right.GetInt()));
        SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False());
    } else if (left.IsNumber() && right.IsNumber()) {
        // fast path
        double valueA = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
        double valueB = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
        ComparisonResult comparison = JSTaggedValue::StrictNumberCompare(valueA, valueB);
        bool ret = (comparison == ComparisonResult::GREAT) || (comparison == ComparisonResult::EQUAL);
        SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False());
    } else if (left.IsBigInt() && right.IsBigInt()) {
        bool result = BigInt::LessThan(right, left) || BigInt::Equal(right, left);
        SET_ACC(JSTaggedValue(result));
    }  else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::GreaterEq(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(GREATEREQ_IMM8_V8);
}

void InterpreterAssembly::HandleShl2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::shl2"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        int32_t opNumber0 = left.GetInt();
        int32_t opNumber1 = right.GetInt();
        uint32_t shift =
            static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
        using unsigned_type = std::make_unsigned_t<int32_t>;
        auto ret =
            static_cast<int32_t>(static_cast<unsigned_type>(opNumber0) << shift); // NOLINT(hicpp-signed-bitwise)
        SET_ACC(JSTaggedValue(ret));
    } else if (left.IsNumber() && right.IsNumber()) {
        int32_t opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        int32_t opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
        uint32_t shift =
            static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
        using unsigned_type = std::make_unsigned_t<int32_t>;
        auto ret =
            static_cast<int32_t>(static_cast<unsigned_type>(opNumber0) << shift); // NOLINT(hicpp-signed-bitwise)
        SET_ACC(JSTaggedValue(ret));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Shl2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(SHL2_IMM8_V8);
}

void InterpreterAssembly::HandleShr2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::shr2"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        int32_t opNumber0 = left.GetInt();
        int32_t opNumber1 = right.GetInt();
        uint32_t shift =
            static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
        using unsigned_type = std::make_unsigned_t<uint32_t>;
        auto ret =
            static_cast<uint32_t>(static_cast<unsigned_type>(opNumber0) >> shift); // NOLINT(hicpp-signed-bitwise)
        SET_ACC(JSTaggedValue(ret));
    } else if (left.IsNumber() && right.IsNumber()) {
        int32_t opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        int32_t opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
        uint32_t shift =
            static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
        using unsigned_type = std::make_unsigned_t<uint32_t>;
        auto ret =
            static_cast<uint32_t>(static_cast<unsigned_type>(opNumber0) >> shift); // NOLINT(hicpp-signed-bitwise)
        SET_ACC(JSTaggedValue(ret));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Shr2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(SHR2_IMM8_V8);
}

void InterpreterAssembly::HandleAshr2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::ashr2"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        int32_t opNumber0 = left.GetInt();
        int32_t opNumber1 = right.GetInt();
        uint32_t shift =
            static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
        auto ret = static_cast<int32_t>(opNumber0 >> shift); // NOLINT(hicpp-signed-bitwise)
        SET_ACC(JSTaggedValue(ret));
    } else if (left.IsNumber() && right.IsNumber()) {
        int32_t opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        int32_t opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
        uint32_t shift =
            static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
        auto ret = static_cast<int32_t>(opNumber0 >> shift); // NOLINT(hicpp-signed-bitwise)
        SET_ACC(JSTaggedValue(ret));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Ashr2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(ASHR2_IMM8_V8);
}

void InterpreterAssembly::HandleAnd2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::and2"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        int32_t opNumber0 = left.GetInt();
        int32_t opNumber1 = right.GetInt();
        // NOLINT(hicpp-signed-bitwise)
        auto ret = static_cast<uint32_t>(opNumber0) & static_cast<uint32_t>(opNumber1);
        SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    } else if (left.IsNumber() && right.IsNumber()) {
        int32_t opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        int32_t opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
        // NOLINT(hicpp-signed-bitwise)
        auto ret = static_cast<uint32_t>(opNumber0) & static_cast<uint32_t>(opNumber1);
        SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::And2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(AND2_IMM8_V8);
}

void InterpreterAssembly::HandleOr2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::or2"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        int32_t opNumber0 = left.GetInt();
        int32_t opNumber1 = right.GetInt();
        // NOLINT(hicpp-signed-bitwise)
        auto ret = static_cast<uint32_t>(opNumber0) | static_cast<uint32_t>(opNumber1);
        SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    } else if (left.IsNumber() && right.IsNumber()) {
        int32_t opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        int32_t opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
        // NOLINT(hicpp-signed-bitwise)
        auto ret = static_cast<uint32_t>(opNumber0) | static_cast<uint32_t>(opNumber1);
        SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Or2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(OR2_IMM8_V8);
}

void InterpreterAssembly::HandleXor2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::xor2"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        int32_t opNumber0 = left.GetInt();
        int32_t opNumber1 = right.GetInt();
        // NOLINT(hicpp-signed-bitwise)
        auto ret = static_cast<uint32_t>(opNumber0) ^ static_cast<uint32_t>(opNumber1);
        SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    } else if (left.IsNumber() && right.IsNumber()) {
        int32_t opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        int32_t opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
        // NOLINT(hicpp-signed-bitwise)
        auto ret = static_cast<uint32_t>(opNumber0) ^ static_cast<uint32_t>(opNumber1);
        SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Xor2(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(XOR2_IMM8_V8);
}

void InterpreterAssembly::HandleDelobjpropV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsics::delobjprop"
               << " v0" << v0;

    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue prop = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::DelObjProp(thread, obj, prop);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);

    DISPATCH(DELOBJPROP_V8);
}

void InterpreterAssembly::HandleExpImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::exp"
               << " v" << v0;
    JSTaggedValue base = GET_VREG_VALUE(v0);
    JSTaggedValue exponent = GET_ACC();
    if (base.IsNumber() && exponent.IsNumber()) {
        // fast path
        double doubleBase = base.IsInt() ? base.GetInt() : base.GetDouble();
        double doubleExponent = exponent.IsInt() ? exponent.GetInt() : exponent.GetDouble();
        if ((std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) || std::isnan(doubleExponent)) {
            SET_ACC(JSTaggedValue(base::NAN_VALUE));
        }
        bool baseZero = doubleBase == 0 &&
            (base::bit_cast<uint64_t>(doubleBase) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK;
        bool isFinite = std::isfinite(doubleExponent);
        bool truncEqual = base::NumberHelper::TruncateDouble(doubleExponent) == doubleExponent;
        bool halfTruncEqual = (base::NumberHelper::TruncateDouble(doubleExponent / 2) + base::HALF) ==
                                (doubleExponent / 2);
        if (baseZero && isFinite && truncEqual && halfTruncEqual) {
            if (doubleExponent > 0) {
                SET_ACC(JSTaggedValue(-0.0));
            }
            if (doubleExponent < 0) {
                SET_ACC(JSTaggedValue(-base::POSITIVE_INFINITY));
            }
        }
        SET_ACC(JSTaggedValue(std::pow(doubleBase, doubleExponent)));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Exp(thread, base, exponent);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(EXP_IMM8_V8);
}

void InterpreterAssembly::HandleIsinImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::isin"
               << " v" << v0;
    JSTaggedValue prop = GET_VREG_VALUE(v0);
    JSTaggedValue obj = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::IsIn(thread, prop, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(ISIN_IMM8_V8);
}

void InterpreterAssembly::HandleInstanceofImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::instanceof"
               << " v" << v0;
    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue target = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::Instanceof(thread, obj, target);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(INSTANCEOF_IMM8_V8);
}

void InterpreterAssembly::HandleStrictnoteqImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::strictnoteq"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    JSTaggedValue res = FastRuntimeStub::FastStrictEqual(left, right);
    if (!res.IsHole()) {
        res = res.IsTrue() ? JSTaggedValue::False() : JSTaggedValue::True();
        SET_ACC(res);
    } else {
        // slow path
        res = SlowRuntimeStub::NotEq(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(STRICTNOTEQ_IMM8_V8);
}

void InterpreterAssembly::HandleStricteqImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::stricteq"
               << " v" << v0;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    JSTaggedValue res = FastRuntimeStub::FastStrictEqual(left, right);
    if (!res.IsHole()) {
        SET_ACC(res);
    } else {
        // slow path
        res = SlowRuntimeStub::Eq(thread, left, right);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(STRICTEQ_IMM8_V8);
}

void InterpreterAssembly::HandleLdlexvarImm8Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_8_0();
    uint16_t slot = READ_INST_8_1();

    LOG_INST() << "intrinsics::ldlexvar"
               << " level:" << level << " slot:" << slot;
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    JSTaggedValue currentLexenv = state->env;
    JSTaggedValue env(currentLexenv);
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SET_ACC(LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
    DISPATCH(LDLEXVAR_IMM8_IMM8);
}

void InterpreterAssembly::HandleLdlexvarImm4Imm4(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_4_0();
    uint16_t slot = READ_INST_4_1();

    LOG_INST() << "intrinsics::ldlexvar"
               << " level:" << level << " slot:" << slot;
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    JSTaggedValue currentLexenv = state->env;
    JSTaggedValue env(currentLexenv);
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SET_ACC(LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
    DISPATCH(LDLEXVAR_IMM4_IMM4);
}

void InterpreterAssembly::HandleWideStlexvarPrefImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_16_1();
    uint16_t slot = READ_INST_16_3();
    LOG_INST() << "intrinsics::stlexvar"
               << " level:" << level << " slot:" << slot;

    JSTaggedValue value = GET_ACC();
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    JSTaggedValue env = state->env;
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(WIDE_STLEXVAR_PREF_IMM16_IMM16);
}

void InterpreterAssembly::HandleStlexvarImm8Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_8_0();
    uint16_t slot = READ_INST_8_1();
    LOG_INST() << "intrinsics::stlexvar"
               << " level:" << level << " slot:" << slot;

    JSTaggedValue value = GET_ACC();
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    JSTaggedValue env = state->env;
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(STLEXVAR_IMM8_IMM8);
}

void InterpreterAssembly::HandleStlexvarImm4Imm4(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_4_0();
    uint16_t slot = READ_INST_4_1();
    LOG_INST() << "intrinsics::stlexvar"
               << " level:" << level << " slot:" << slot;

    JSTaggedValue value = GET_ACC();
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    JSTaggedValue env = state->env;
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(STLEXVAR_IMM4_IMM4);
}

void InterpreterAssembly::HandleNewlexenvImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint8_t numVars = READ_INST_8_0();
    LOG_INST() << "intrinsics::newlexenv"
               << " imm " << numVars;
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = FastRuntimeStub::NewLexicalEnv(thread, factory, numVars);
    if (res.IsHole()) {
        res = SlowRuntimeStub::NewLexicalEnv(thread, numVars);
        INTERPRETER_RETURN_IF_ABRUPT(res);
    }
    SET_ACC(res);
    GET_ASM_FRAME(sp)->env = res;
    DISPATCH(NEWLEXENV_IMM8);
}

void InterpreterAssembly::HandlePoplexenv(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    JSTaggedValue currentLexenv = state->env;
    JSTaggedValue parentLexenv = LexicalEnv::Cast(currentLexenv.GetTaggedObject())->GetParentEnv();
    GET_ASM_FRAME(sp)->env = parentLexenv;
    DISPATCH(POPLEXENV);
}

void InterpreterAssembly::HandleCreateiterresultobjV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    uint16_t v1 = READ_INST_8_1();
    LOG_INST() << "intrinsics::createiterresultobj"
               << " v" << v0 << " v" << v1;
    JSTaggedValue value = GET_VREG_VALUE(v0);
    JSTaggedValue flag = GET_VREG_VALUE(v1);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CreateIterResultObj(thread, value, flag);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEITERRESULTOBJ_V8_V8);
}

void InterpreterAssembly::HandleSuspendgeneratorV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsics::suspendgenerator"
                << " v" << v0;
    JSTaggedValue genObj = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_ACC();
    // suspend will record bytecode offset
    SAVE_PC();
    SAVE_ACC();
    JSTaggedValue res = SlowRuntimeStub::SuspendGenerator(thread, genObj, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);

    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    Method *method = ECMAObject::Cast(state->function.GetTaggedObject())->GetCallTarget();
    [[maybe_unused]] auto fistPC = method->GetBytecodeArray();
    UPDATE_HOTNESS_COUNTER(-(pc - fistPC));
    LOG_INST() << "Exit: SuspendGenerator " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                            << std::hex << reinterpret_cast<uintptr_t>(state->pc);
    sp = state->base.prev;
    ASSERT(sp != nullptr);

    AsmInterpretedFrame *prevState = GET_ASM_FRAME(sp);
    pc = prevState->pc;
    thread->SetCurrentSPFrame(sp);
    // entry frame
    if (pc == nullptr) {
        state->acc = acc;
        return;
    }

    ASSERT(prevState->callSize == GetJumpSizeAfterCall(pc));
    DISPATCH_OFFSET(prevState->callSize);
}

void InterpreterAssembly::HandleAsyncfunctionawaituncaughtV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsics::asyncfunctionawaituncaught"
               << " v" << v0;
    JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncFunctionAwaitUncaught(thread, asyncFuncObj, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(ASYNCFUNCTIONAWAITUNCAUGHT_V8);
}

void InterpreterAssembly::HandleAsyncfunctionresolveV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsics::asyncfunctionresolve"
                << " v" << v0;

    JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncFunctionResolveOrReject(thread, asyncFuncObj, value, true);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(ASYNCFUNCTIONRESOLVE_V8);
}

void InterpreterAssembly::HandleAsyncfunctionrejectV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
        uint16_t v0 = READ_INST_8_0();
        LOG_INST() << "intrinsics::asyncfunctionreject"
                   << " v" << v0;

        JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::AsyncFunctionResolveOrReject(thread, asyncFuncObj, value, false);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(ASYNCFUNCTIONREJECT_V8);
}

void InterpreterAssembly::HandleNewobjapplyImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsic::newobjspeard"
               << " v" << v0;
    JSTaggedValue func = GET_VREG_VALUE(v0);
    JSTaggedValue array = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewObjApply(thread, func, array);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(NEWOBJAPPLY_IMM8_V8);
}

void InterpreterAssembly::HandleThrowUndefinedifholePrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsic::throwundefinedifhole"
               << " v" << v0 << " v" << v1;
    JSTaggedValue hole = GET_VREG_VALUE(v0);
    if (!hole.IsHole()) {
        DISPATCH(THROW_UNDEFINEDIFHOLE_PREF_V8_V8);
    }
    JSTaggedValue obj = GET_VREG_VALUE(v1);
    ASSERT(obj.IsString());
    SAVE_PC();
    SlowRuntimeStub::ThrowUndefinedIfHole(thread, obj);
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleThrowUndefinedifholewithnamePrefId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    JSTaggedValue hole = acc;
    if (!hole.IsHole()) {
        DISPATCH(THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16);
    }

    uint16_t stringId = READ_INST_16_1();
    LOG_INST() << "intrinsic::throwundefinedifholewithname" << std::hex << stringId;
    constpool = GetConstantPool(sp);
    JSTaggedValue obj = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    ASSERT(obj.IsString());
    SAVE_PC();
    SlowRuntimeStub::ThrowUndefinedIfHole(thread, obj);
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleStownbynameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    uint32_t v0 = READ_INST_8_3();
    LOG_INST() << "intrinsics::stownbyname "
               << "v" << v0 << " stringId:" << stringId;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsJSObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        constpool = GetConstantPool(sp);
        JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
        RESTORE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        SAVE_ACC();
        receiver = GET_VREG_VALUE(v0);
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOWNBYNAME_IMM8_ID16_V8);
        }
        RESTORE_ACC();
    }

    SAVE_ACC();
    constpool = GetConstantPool(sp);
    auto propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);  // Maybe moved by GC
    RESTORE_ACC();
    auto value = GET_ACC();                                  // Maybe moved by GC
    receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StOwnByName(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleCreateemptyarrayImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::createemptyarray";
    SAVE_PC();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateEmptyArray(thread, factory, globalEnv);
    SET_ACC(res);
    DISPATCH(CREATEEMPTYARRAY_IMM8);
}

void InterpreterAssembly::HandleCreateemptyobject(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::createemptyobject";
    SAVE_PC();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateEmptyObject(thread, factory, globalEnv);
    SET_ACC(res);
    DISPATCH(CREATEEMPTYOBJECT);
}

void InterpreterAssembly::HandleSetobjectwithprotoImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::setobjectwithproto"
               << " v" << v0;
    JSTaggedValue proto = GET_VREG_VALUE(v0);
    JSTaggedValue obj = GET_ACC();
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::SetObjectWithProto(thread, proto, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(SETOBJECTWITHPROTO_IMM8_V8);
}

void InterpreterAssembly::HandleCreateregexpwithliteralImm8Id16Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue pattern = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    uint8_t flags = READ_INST_8_3();
    LOG_INST() << "intrinsics::createregexpwithliteral "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(pattern.GetTaggedObject()))
               << ", flags:" << flags;
    JSTaggedValue res = SlowRuntimeStub::CreateRegExpWithLiteral(thread, pattern, flags);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8);
}

void InterpreterAssembly::HandleGettemplateobjectImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsic::gettemplateobject";

    JSTaggedValue literal = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetTemplateObject(thread, literal);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETTEMPLATEOBJECT_IMM8);
}

void InterpreterAssembly::HandleGetnextpropnameV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsic::getnextpropname"
                << " v" << v0;
    JSTaggedValue iter = GET_VREG_VALUE(v0);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetNextPropName(thread, iter);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETNEXTPROPNAME_V8);
}

void InterpreterAssembly::HandleCopydatapropertiesV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsic::copydataproperties"
               << " v" << v0;
    JSTaggedValue dst = GET_VREG_VALUE(v0);
    JSTaggedValue src = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CopyDataProperties(thread, dst, src);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(COPYDATAPROPERTIES_V8);
}

void InterpreterAssembly::HandleStownbyindexImm8V8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint16_t index = READ_INST_16_2();
    LOG_INST() << "intrinsics::stownbyindex"
               << " v" << v0 << " imm" << index;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    // fast path
    if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByIndex<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, index, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOWNBYINDEX_IMM8_V8_IMM16);
        }
        RESTORE_ACC();
    }
    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
    auto value = GET_ACC();         // Maybe moved by GC
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StOwnByIndex(thread, receiver, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOWNBYINDEX_IMM8_V8_IMM16);
}

void InterpreterAssembly::HandleStownbyvalueImm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint32_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::stownbyvalue"
               << " v" << v0 << " v" << v1;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, propKey, value);

        // SetPropertyByValue maybe gc need update the value
        RESTORE_ACC();
        propKey = GET_VREG_VALUE(v1);
        value = GET_ACC();
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOWNBYVALUE_IMM8_V8_V8);
        }
    }

    // slow path
    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);      // Maybe moved by GC
    auto propKey = GET_VREG_VALUE(v1);  // Maybe moved by GC
    auto value = GET_ACC();             // Maybe moved by GC
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StOwnByValue(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYVALUE_IMM8_V8_V8);
}

void InterpreterAssembly::HandleCreateobjectwithexcludedkeysImm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint8_t numKeys = READ_INST_8_0();
    uint16_t v0 = READ_INST_8_1();
    uint16_t firstArgRegIdx = READ_INST_8_2();
    LOG_INST() << "intrinsics::createobjectwithexcludedkeys " << numKeys << " v" << firstArgRegIdx;

    JSTaggedValue obj = GET_VREG_VALUE(v0);

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CreateObjectWithExcludedKeys(thread, numKeys, obj, firstArgRegIdx);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8);
}

void InterpreterAssembly::HandleLdhole(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsic::ldhole";
    SET_ACC(JSTaggedValue::Hole());
    DISPATCH(LDHOLE);
}

void InterpreterAssembly::HandleCopyrestargsImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t restIdx = READ_INST_8_0();
    LOG_INST() << "intrinsics::copyrestargs"
               << " index: " << restIdx;

    uint32_t startIdx = 0;
    uint32_t restNumArgs = GetNumArgs(sp, restIdx, startIdx);

    JSTaggedValue res = SlowRuntimeStub::CopyRestArgs(thread, sp, restNumArgs, startIdx);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(COPYRESTARGS_IMM8);
}

void InterpreterAssembly::HandleDefinegettersetterbyvalueV8V8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    uint16_t v1 = READ_INST_8_1();
    uint16_t v2 = READ_INST_8_2();
    uint16_t v3 = READ_INST_8_3();
    LOG_INST() << "intrinsics::definegettersetterbyvalue"
               << " v" << v0 << " v" << v1 << " v" << v2 << " v" << v3;

    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue prop = GET_VREG_VALUE(v1);
    JSTaggedValue getter = GET_VREG_VALUE(v2);
    JSTaggedValue setter = GET_VREG_VALUE(v3);
    JSTaggedValue flag = GET_ACC();
    SAVE_PC();
    JSTaggedValue res =
        SlowRuntimeStub::DefineGetterSetterByValue(thread, obj, prop, getter, setter, flag.ToBoolean());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8);
}

void InterpreterAssembly::HandleStobjbyindexImm8V8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint32_t index = READ_INST_16_2();
    LOG_INST() << "intrinsics::stobjbyindex"
                << " v" << v0 << " imm" << index;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByIndex(thread, receiver, index, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYINDEX_IMM8_V8_IMM16);
        }
        RESTORE_ACC();
    }
    // slow path
    SAVE_ACC();
    SAVE_PC();
    receiver = GET_VREG_VALUE(v0);    // Maybe moved by GC
    JSTaggedValue value = GET_ACC();  // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StObjByIndex(thread, receiver, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOBJBYINDEX_IMM8_V8_IMM16);
}

void InterpreterAssembly::HandleStobjbyvalueImm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint32_t v1 = READ_INST_8_2();

    LOG_INST() << "intrinsics::stobjbyvalue"
               << " v" << v0 << " v" << v1;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByValue(thread, receiver, propKey, firstValue, secondValue, value);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::StoreICByValue(thread,
                                                profileTypeArray,
                                                receiver, propKey, value, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYVALUE_IMM8_V8_V8);
        }
    }
#endif
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYVALUE_IMM8_V8_V8);
        }
        RESTORE_ACC();
    }
    {
        // slow path
        SAVE_ACC();
        SAVE_PC();
        receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
        JSTaggedValue propKey = GET_VREG_VALUE(v1);   // Maybe moved by GC
        JSTaggedValue value = GET_ACC();              // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StObjByValue(thread, receiver, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    }
    DISPATCH(STOBJBYVALUE_IMM8_V8_V8);
}

void InterpreterAssembly::HandleStsuperbyvalueImm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint32_t v1 = READ_INST_8_2();

    LOG_INST() << "intrinsics::stsuperbyvalue"
               << " v" << v0 << " v" << v1;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_VREG_VALUE(v1);
    JSTaggedValue value = GET_ACC();

    // slow path
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::StSuperByValue(thread, receiver, propKey, value, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STSUPERBYVALUE_IMM8_V8_V8);
}

void InterpreterAssembly::HandleTryldglobalbynameImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    constpool = GetConstantPool(sp);
    auto prop = ConstantPool::GetStringFromCache(thread, constpool, stringId);

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();

    LOG_INST() << "intrinsics::tryldglobalbyname "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(prop.GetTaggedObject()));

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        JSTaggedValue res = ICRuntimeStub::LoadGlobalICByName(thread,
                                                              ProfileTypeInfo::Cast(
                                                                  tmpProfileTypeInfo.GetTaggedObject()),
                                                              globalObj, prop, slotId, true);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(TRYLDGLOBALBYNAME_IMM8_ID16);
    }
#endif

    // order: 1. global record 2. global object
    JSTaggedValue result = SlowRuntimeStub::LdGlobalRecord(thread, prop);
    if (!result.IsUndefined()) {
        SET_ACC(PropertyBox::Cast(result.GetTaggedObject())->GetValue());
    } else {
        JSTaggedValue globalResult = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, prop);
        if (!globalResult.IsHole()) {
            SET_ACC(globalResult);
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::TryLdGlobalByNameFromGlobalProto(thread, globalObj, prop);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
    }

    DISPATCH(TRYLDGLOBALBYNAME_IMM8_ID16);
}

void InterpreterAssembly::HandleTrystglobalbynameImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();

    RESTORE_ACC();
    LOG_INST() << "intrinsics::trystglobalbyname"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        JSTaggedValue res = ICRuntimeStub::StoreGlobalICByName(thread,
                                                               ProfileTypeInfo::Cast(
                                                                   tmpProfileTypeInfo.GetTaggedObject()),
                                                               globalObj, propKey, value, slotId, true);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(TRYSTGLOBALBYNAME_IMM8_ID16);
    }
#endif

    auto recordResult = SlowRuntimeStub::LdGlobalRecord(thread, propKey);
    SAVE_PC();
    // 1. find from global record
    if (!recordResult.IsUndefined()) {
        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        JSTaggedValue res = SlowRuntimeStub::TryUpdateGlobalRecord(thread, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    } else {
        // 2. find from global object
        auto globalResult = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, propKey);
        if (globalResult.IsHole()) {
            auto result = SlowRuntimeStub::ThrowReferenceError(thread, propKey, " is not defined");
            INTERPRETER_RETURN_IF_ABRUPT(result);
        }
        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        JSTaggedValue res = SlowRuntimeStub::StGlobalVar(thread, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    }
    DISPATCH(TRYSTGLOBALBYNAME_IMM8_ID16);
}

void InterpreterAssembly::HandleStownbyvaluewithnamesetImm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint32_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::stownbyvaluewithnameset"
               << " v" << v0 << " v" << v1;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, propKey, value);

        // SetPropertyByValue maybe gc need update the value
        RESTORE_ACC();
        propKey = GET_VREG_VALUE(v1);
        value = GET_ACC();
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            JSFunction::SetFunctionNameNoPrefix(thread, JSFunction::Cast(value.GetTaggedObject()), propKey);
            RESTORE_ACC();
            DISPATCH(STOWNBYVALUEWITHNAMESET_IMM8_V8_V8);
        }
    }

    // slow path
    SAVE_ACC();
    SAVE_PC();
    receiver = GET_VREG_VALUE(v0);      // Maybe moved by GC
    auto propKey = GET_VREG_VALUE(v1);  // Maybe moved by GC
    auto value = GET_ACC();             // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StOwnByValueWithNameSet(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYVALUEWITHNAMESET_IMM8_V8_V8);
}

void InterpreterAssembly::HandleStownbynamewithnamesetImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    uint32_t v0 = READ_INST_8_3();
    LOG_INST() << "intrinsics::stownbynamewithnameset "
                << "v" << v0 << " stringId:" << stringId;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsJSObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        constpool = GetConstantPool(sp);
        JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
        RESTORE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        SAVE_ACC();
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            JSFunction::SetFunctionNameNoPrefix(thread, JSFunction::Cast(value.GetTaggedObject()), propKey);
            RESTORE_ACC();
            DISPATCH(STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8);
        }
        RESTORE_ACC();
    }

    SAVE_ACC();
    SAVE_PC();
    receiver = GET_VREG_VALUE(v0);
    constpool = GetConstantPool(sp);                           // Maybe moved by GC
    auto propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);  // Maybe moved by GC
    RESTORE_ACC();
    auto value = GET_ACC();                                  // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StOwnByNameWithNameSet(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleLdglobalvarImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    LOG_INST() << "intrinsics::ldglobalvar stringId:" << stringId;
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        JSTaggedValue res = ICRuntimeStub::LoadGlobalICByName(thread,
                                                              ProfileTypeInfo::Cast(
                                                                  tmpProfileTypeInfo.GetTaggedObject()),
                                                              globalObj, propKey, slotId, false);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(LDGLOBALVAR_IMM16_ID16);
    }
#endif

    JSTaggedValue result = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, propKey);
    if (!result.IsHole()) {
        SET_ACC(result);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::LdGlobalVarFromGlobalProto(thread, globalObj, propKey);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(LDGLOBALVAR_IMM16_ID16);
}

void InterpreterAssembly::HandleStobjbynameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_3();
#if ECMASCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_ACC();
        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByName(thread, receiver, firstValue, secondValue, value);
        }
        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYNAME_IMM8_ID16_V8);
        } else if (!firstValue.IsHole()) { // IC miss and not enter the megamorphic state, store as polymorphic
            uint16_t stringId = READ_INST_16_1();
            SAVE_ACC();
            constpool = GetConstantPool(sp);
            JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
            RESTORE_ACC();
            value = GET_ACC();
            receiver = GET_VREG_VALUE(v0);
            profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
            res = ICRuntimeStub::StoreICByName(thread,
                                               profileTypeArray,
                                               receiver, propKey, value, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYNAME_IMM8_ID16_V8);
        }
    }
#endif
    uint16_t stringId = READ_INST_16_1();
    LOG_INST() << "intrinsics::stobjbyname "
               << "v" << v0 << " stringId:" << stringId;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        constpool = GetConstantPool(sp);
        JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
        RESTORE_ACC();
        JSTaggedValue value = GET_ACC();
        receiver = GET_VREG_VALUE(v0);
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYNAME_IMM8_ID16_V8);
        }
        RESTORE_ACC();
    }
    // slow path
    SAVE_ACC();
    SAVE_PC();
    constpool = GetConstantPool(sp);                    // Maybe moved by GC
    auto propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);  // Maybe moved by GC
    RESTORE_ACC();
    JSTaggedValue value = GET_ACC();                                  // Maybe moved by GC
    receiver = GET_VREG_VALUE(v0);
    JSTaggedValue res = SlowRuntimeStub::StObjByName(thread, receiver, propKey, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOBJBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleStsuperbynameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    uint32_t v0 = READ_INST_8_3();

    JSTaggedValue obj = GET_VREG_VALUE(v0);
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    JSTaggedValue value = GET_ACC();

    LOG_INST() << "intrinsics::stsuperbyname"
               << "v" << v0 << " stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData()
               << ", value:" << value.GetRawData();

    // slow path
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::StSuperByValue(thread, obj, propKey, value, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STSUPERBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleStglobalvarImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue prop = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    JSTaggedValue value = GET_ACC();

    LOG_INST() << "intrinsics::stglobalvar "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(prop.GetTaggedObject()))
               << ", value:" << value.GetRawData();
#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        SAVE_ACC();
        JSTaggedValue res = ICRuntimeStub::StoreGlobalICByName(thread,
                                                               ProfileTypeInfo::Cast(
                                                                   tmpProfileTypeInfo.GetTaggedObject()),
                                                               globalObj, prop, value, slotId, false);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(STGLOBALVAR_IMM16_ID16);
    }
#endif
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalVar(thread, prop, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STGLOBALVAR_IMM16_ID16);
}

void InterpreterAssembly::HandleCreategeneratorobjV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsics::creategeneratorobj"
               << " v" << v0;
    SAVE_PC();
    JSTaggedValue genFunc = GET_VREG_VALUE(v0);
    JSTaggedValue res = SlowRuntimeStub::CreateGeneratorObj(thread, genFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEGENERATOROBJ_V8);
}

void InterpreterAssembly::HandleCreateasyncgeneratorobjV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsics::createasyncgeneratorobj"
               << " v" << v0;
    SAVE_PC();
    JSTaggedValue genFunc = GET_VREG_VALUE(v0);
    JSTaggedValue res = SlowRuntimeStub::CreateAsyncGeneratorObj(thread, genFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEASYNCGENERATOROBJ_V8);
}

void InterpreterAssembly::HandleAsyncgeneratorresolveV8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    uint16_t v1 = READ_INST_8_1();
    uint16_t v2 = READ_INST_8_2();
    LOG_INST() << "intrinsics::asyncgeneratorresolve"
                   << " v" << v0 << " v" << v1 << " v" << v2;
    JSTaggedValue asyncGenerator = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_VREG_VALUE(v1);
    JSTaggedValue flag = GET_VREG_VALUE(v2);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncGeneratorResolve(thread, asyncGenerator, value, flag);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);

    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget();
    [[maybe_unused]] auto fistPC = method->GetBytecodeArray();
    UPDATE_HOTNESS_COUNTER(-(pc - fistPC));
    LOG_INST() << "Exit: AsyncGeneratorresolve " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                            << std::hex << reinterpret_cast<uintptr_t>(state->pc);
    sp = state->base.prev;
    ASSERT(sp != nullptr);
    InterpretedFrame *prevState = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    pc = prevState->pc;
    // entry frame
    if (FrameHandler::IsEntryFrame(pc)) {
        state->acc = acc;
        return;
    }

    thread->SetCurrentSPFrame(sp);

    size_t jumpSize = GetJumpSizeAfterCall(pc);
    DISPATCH_OFFSET(jumpSize);
}

void InterpreterAssembly::HandleAsyncgeneratorrejectV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsics::asyncgeneratorreject"
               << " v" << v0;
    JSTaggedValue asyncGenerator = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncGeneratorReject(thread, asyncGenerator, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(ASYNCGENERATORREJECT_V8);
}

void InterpreterAssembly::HandleSetgeneratorstateImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t index = READ_INST_8_0();
    LOG_INST() << "intrinsics::setgeneratorstate index" << index;
    JSTaggedValue objVal = GET_ACC();

    SAVE_PC();
    SAVE_ACC();
    SlowRuntimeStub::SetGeneratorState(thread, objVal, index);
    RESTORE_ACC();
    DISPATCH(SETGENERATORSTATE_IMM8);
}

void InterpreterAssembly::HandleDeprecatedAsyncgeneratorrejectPrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::asyncgeneratorreject"
                   << " v" << v0 << " v" << v1;
    JSTaggedValue asyncGenerator = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_VREG_VALUE(v1);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncGeneratorReject(thread, asyncGenerator, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_ASYNCGENERATORREJECT_PREF_V8_V8);
}

void InterpreterAssembly::HandleStarrayspreadV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    uint16_t v1 = READ_INST_8_1();
    LOG_INST() << "ecmascript::intrinsics::starrayspread"
               << " v" << v0 << " v" << v1 << "acc";
    JSTaggedValue dst = GET_VREG_VALUE(v0);
    JSTaggedValue index = GET_VREG_VALUE(v1);
    JSTaggedValue src = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StArraySpread(thread, dst, index, src);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(STARRAYSPREAD_V8_V8);
}

void InterpreterAssembly::HandleLdfunction(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsic::ldfunction";
    SET_ACC(GetFunction(sp));
    DISPATCH(LDFUNCTION);
}

void InterpreterAssembly::HandleLdbigintId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    LOG_INST() << "intrinsic::ldbigint";
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue numberBigInt = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdBigInt(thread, numberBigInt);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDBIGINT_ID16);
}

void InterpreterAssembly::HandleTonumericImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::tonumeric";
    JSTaggedValue value = GET_ACC();
    if (value.IsNumber() || value.IsBigInt()) {
        // fast path
        SET_ACC(value);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::ToNumeric(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(TONUMERIC_IMM8);
}

void InterpreterAssembly::HandleSupercallspreadImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsic::supercallspread"
               << " array: v" << v0;

    JSTaggedValue thisFunc = GET_ACC();
    JSTaggedValue newTarget = GetNewTarget(sp);
    JSTaggedValue array = GET_VREG_VALUE(v0);

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::SuperCallSpread(thread, thisFunc, newTarget, array);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(SUPERCALLSPREAD_IMM8_V8);
}

void InterpreterAssembly::HandleThrowIfsupernotcorrectcallPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    JSTaggedValue thisValue = GET_ACC();
    LOG_INST() << "intrinsic::throwifsupernotcorrectcall"
               << " imm:" << imm;
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::ThrowIfSuperNotCorrectCall(thread, imm, thisValue);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16);
}

void InterpreterAssembly::HandleThrowDeletesuperpropertyPrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "throwdeletesuperproperty";

    SAVE_PC();
    SlowRuntimeStub::ThrowDeleteSuperProperty(thread);
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleDebugger(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::debugger";
    DISPATCH(DEBUGGER);
}

void InterpreterAssembly::HandleIstrue(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::istrue";
    if (GET_ACC().ToBoolean()) {
        SET_ACC(JSTaggedValue::True());
    } else {
        SET_ACC(JSTaggedValue::False());
    }
    DISPATCH(ISTRUE);
}

void InterpreterAssembly::HandleCallRuntimeIstruePrefImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::callruntimeistrueprefimm8";
    if (GET_ACC().ToBoolean()) {
        SET_ACC(JSTaggedValue::True());
    } else {
        SET_ACC(JSTaggedValue::False());
    }
    DISPATCH(CALLRUNTIME_ISTRUE_PREF_IMM8);
}

void InterpreterAssembly::HandleIsfalse(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::isfalse";
    if (GET_ACC().ToBoolean()) {
        SET_ACC(JSTaggedValue::False());
    } else {
        SET_ACC(JSTaggedValue::True());
    }
    DISPATCH(ISFALSE);
}

void InterpreterAssembly::HandleCallRuntimeIsfalsePrefImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::callruntimeisfalseprefimm8";
    if (GET_ACC().ToBoolean()) {
        SET_ACC(JSTaggedValue::False());
    } else {
        SET_ACC(JSTaggedValue::True());
    }
    DISPATCH(CALLRUNTIME_ISFALSE_PREF_IMM8);
}

void InterpreterAssembly::HandleTypeofImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::typeof";
    JSTaggedValue res = FastRuntimeStub::FastTypeOf(thread, GET_ACC());
    SET_ACC(res);
    DISPATCH(TYPEOF_IMM16);
}

void InterpreterAssembly::HandleCreateemptyarrayImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::createemptyarray";
    SAVE_PC();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateEmptyArray(thread, factory, globalEnv);
    SET_ACC(res);
    DISPATCH(CREATEEMPTYARRAY_IMM16);
}

void InterpreterAssembly::HandleGetiteratorImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::getiterator";
    JSTaggedValue obj = GET_ACC();
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetIterator(thread, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETITERATOR_IMM16);
}

void InterpreterAssembly::HandleGettemplateobjectImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::gettemplateobject";
    JSTaggedValue obj = GET_ACC();
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetTemplateObject(thread, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETTEMPLATEOBJECT_IMM16);
}

void InterpreterAssembly::HandleCloseiteratorImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_2();
    LOG_INST() << "intrinsics::closeiterator"
               << " v" << v0;
    SAVE_PC();
    JSTaggedValue iter = GET_VREG_VALUE(v0);
    JSTaggedValue res = SlowRuntimeStub::CloseIterator(thread, iter);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CLOSEITERATOR_IMM16_V8);
}

void InterpreterAssembly::HandleSetobjectwithprotoImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_2();
    LOG_INST() << "intrinsics::setobjectwithproto"
               << " v" << v0;
    JSTaggedValue proto = GET_VREG_VALUE(v0);
    JSTaggedValue obj = GET_ACC();
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::SetObjectWithProto(thread, proto, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(SETOBJECTWITHPROTO_IMM16_V8);
}

void InterpreterAssembly::HandleStobjbyvalueImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_2();
    uint32_t v1 = READ_INST_8_3();

    LOG_INST() << "intrinsics::stobjbyvalue"
               << " v" << v0 << " v" << v1;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByValue(thread, receiver, propKey, firstValue, secondValue, value);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::StoreICByValue(thread,
                                                profileTypeArray,
                                                receiver, propKey, value, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYVALUE_IMM16_V8_V8);
        }
    }
#endif
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYVALUE_IMM16_V8_V8);
        }
        RESTORE_ACC();
    }
    {
        // slow path
        SAVE_ACC();
        SAVE_PC();
        receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
        JSTaggedValue propKey = GET_VREG_VALUE(v1);   // Maybe moved by GC
        JSTaggedValue value = GET_ACC();              // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StObjByValue(thread, receiver, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    }
    DISPATCH(STOBJBYVALUE_IMM16_V8_V8);
}

void InterpreterAssembly::HandleStownbyvalueImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_2();
    uint32_t v1 = READ_INST_8_3();
    LOG_INST() << "intrinsics::stownbyvalue"
               << " v" << v0 << " v" << v1;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, propKey, value);

        // SetPropertyByValue maybe gc need update the value
        RESTORE_ACC();
        propKey = GET_VREG_VALUE(v1);
        value = GET_ACC();
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOWNBYVALUE_IMM16_V8_V8);
        }
    }

    // slow path
    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);      // Maybe moved by GC
    auto propKey = GET_VREG_VALUE(v1);  // Maybe moved by GC
    auto value = GET_ACC();             // Maybe moved by GC
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StOwnByValue(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYVALUE_IMM16_V8_V8);
}

void InterpreterAssembly::HandleStobjbyindexImm16V8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint8_t v0 = READ_INST_8_2();
    uint16_t index = READ_INST_16_3();
    LOG_INST() << "intrinsics::stobjbyindex"
               << " v" << v0 << " imm" << index;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByIndex(thread, receiver, index, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYINDEX_IMM16_V8_IMM16);
        }
        RESTORE_ACC();
    }
    // slow path
    SAVE_ACC();
    SAVE_PC();
    receiver = GET_VREG_VALUE(v0);    // Maybe moved by GC
    JSTaggedValue value = GET_ACC();  // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StObjByIndex(thread, receiver, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOBJBYINDEX_IMM16_V8_IMM16);
}

void InterpreterAssembly::HandleStownbyindexImm16V8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint8_t v0 = READ_INST_8_2();
    uint16_t index = READ_INST_16_3();
    LOG_INST() << "intrinsics::stownbyindex"
               << " v" << v0 << " imm" << index;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    // fast path
    if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByIndex<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, index, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOWNBYINDEX_IMM16_V8_IMM16);
        }
        RESTORE_ACC();
    }
    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
    auto value = GET_ACC();         // Maybe moved by GC
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StOwnByIndex(thread, receiver, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOWNBYINDEX_IMM16_V8_IMM16);
}

void InterpreterAssembly::HandleThrowIfsupernotcorrectcallPrefImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint8_t imm = READ_INST_8_1();
    JSTaggedValue thisValue = GET_ACC();
    LOG_INST() << "intrinsic::throwifsupernotcorrectcall"
               << " imm:" << imm;
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::ThrowIfSuperNotCorrectCall(thread, imm, thisValue);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8);
}

void InterpreterAssembly::HandleThrowNotexistsPrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "throwthrownotexists";

    SAVE_PC();
    SlowRuntimeStub::ThrowThrowNotExists(thread);
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleThrowPrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::throw";
    SAVE_PC();
    SlowRuntimeStub::Throw(thread, GET_ACC());
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleWideLdexternalmodulevarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_16_1();

    LOG_INST() << "intrinsics::ldmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdExternalModuleVar(thread, index);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(WIDE_LDEXTERNALMODULEVAR_PREF_IMM16);
}

void InterpreterAssembly::HandleCallRuntimeWideLdsendableexternalmodulevarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_16_1();
    JSTaggedValue thisFunc = GetFunction(sp);
    LOG_INST() << "intrinsics::ldsendableexternalmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdSendableExternalModuleVar(thread, index, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(CALLRUNTIME_WIDELDSENDABLEEXTERNALMODULEVAR_PREF_IMM16);
}

void InterpreterAssembly::HandleCallRuntimeWideLdsendablelocalmodulevarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_16_1();
    JSTaggedValue thisFunc = GetFunction(sp);
    LOG_INST() << "intrinsics::ldsendablelocalmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdSendableLocalModuleVar(thread, index, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(CALLRUNTIME_WIDELDSENDABLELOCALMODULEVAR_PREF_IMM16);
}

void InterpreterAssembly::HandleWideLdlocalmodulevarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_16_1();
    LOG_INST() << "intrinsics::ldmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdLocalModuleVar(thread, index);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(WIDE_LDLOCALMODULEVAR_PREF_IMM16);
}

void InterpreterAssembly::HandleWideStmodulevarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_16_1();

    LOG_INST() << "intrinsics::stmodulevar index:" << index;

    JSTaggedValue value = GET_ACC();

    SlowRuntimeStub::StModuleVar(thread, index, value);
    RESTORE_ACC();
    DISPATCH(WIDE_STMODULEVAR_PREF_IMM16);
}

void InterpreterAssembly::HandleWideGetmodulenamespacePrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_16_1();

    LOG_INST() << "intrinsics::getmodulenamespace index:" << index;

    JSTaggedValue moduleNamespace = SlowRuntimeStub::GetModuleNamespace(thread, index);
    INTERPRETER_RETURN_IF_ABRUPT(moduleNamespace);
    SET_ACC(moduleNamespace);
    DISPATCH(WIDE_GETMODULENAMESPACE_PREF_IMM16);
}

void InterpreterAssembly::HandleWideLdlexvarPrefImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_16_1();
    uint16_t slot = READ_INST_16_3();

    LOG_INST() << "intrinsics::ldlexvar"
               << " level:" << level << " slot:" << slot;
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    JSTaggedValue currentLexenv = state->env;
    JSTaggedValue env(currentLexenv);
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SET_ACC(LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
    DISPATCH(WIDE_LDLEXVAR_PREF_IMM16_IMM16);
}

void InterpreterAssembly::HandleWideCopyrestargsPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t restIdx = READ_INST_16_1();
    LOG_INST() << "intrinsics::copyrestargs"
               << " index: " << restIdx;

    uint32_t startIdx = 0;
    uint32_t restNumArgs = GetNumArgs(sp, restIdx, startIdx);

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CopyRestArgs(thread, sp, restNumArgs, startIdx);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(WIDE_COPYRESTARGS_PREF_IMM16);
}

void InterpreterAssembly::HandleWideStownbyindexPrefV8Imm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint8_t v0 = READ_INST_8_1();
    uint32_t index = READ_INST_32_2();
    LOG_INST() << "intrinsics::stownbyindex"
               << " v" << v0 << " imm" << index;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    // fast path
    if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByIndex<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, index, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(WIDE_STOWNBYINDEX_PREF_V8_IMM32);
        }
        RESTORE_ACC();
    }
    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
    auto value = GET_ACC();         // Maybe moved by GC
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StOwnByIndex(thread, receiver, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(WIDE_STOWNBYINDEX_PREF_V8_IMM32);
}

void InterpreterAssembly::HandleWideStobjbyindexPrefV8Imm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint8_t v0 = READ_INST_8_1();
    uint32_t index = READ_INST_32_2();
    LOG_INST() << "intrinsics::stobjbyindex"
               << " v" << v0 << " imm" << index;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByIndex(thread, receiver, index, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(WIDE_STOBJBYINDEX_PREF_V8_IMM32);
        }
        RESTORE_ACC();
    }
    // slow path
    SAVE_ACC();
    SAVE_PC();
    receiver = GET_VREG_VALUE(v0);    // Maybe moved by GC
    JSTaggedValue value = GET_ACC();  // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StObjByIndex(thread, receiver, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(WIDE_STOBJBYINDEX_PREF_V8_IMM32);
}

void InterpreterAssembly::HandleWideLdobjbyindexPrefImm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t idx = READ_INST_32_1();
    LOG_INST() << "intrinsics::ldobjbyindex"
               << " imm" << idx;

    JSTaggedValue receiver = GET_ACC();
    // fast path
    if (LIKELY(receiver.IsHeapObject())) {
        JSTaggedValue res = FastRuntimeStub::GetPropertyByIndex(thread, receiver, idx);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(WIDE_LDOBJBYINDEX_PREF_IMM32);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByIndex(thread, receiver, idx, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(WIDE_LDOBJBYINDEX_PREF_IMM32);
}

bool InterpreterAssembly::AssemblyIsFastNewFrameEnter(JSFunction *ctor, JSHandle<Method> method)
{
    if (method->IsNativeWithCallField()) {
        return false;
    }

    if (ctor->IsBase()) {
        return method->OnlyHaveThisWithCallField();
    }

    if (ctor->IsDerivedConstructor()) {
        return method->OnlyHaveNewTagetAndThisWithCallField();
    }

    return false;
}

void InterpreterAssembly::HandleWideSupercallarrowrangePrefImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    JSMutableHandle<Method> methodHandle(thread, JSTaggedValue::Undefined());
    uint16_t range = READ_INST_16_1();
    uint16_t v0 = READ_INST_8_3();
    LOG_INST() << "intrinsics::supercall"
               << " range: " << range << " v" << v0;

    JSTaggedValue thisFunc = GET_ACC();
    JSTaggedValue newTarget = GetNewTarget(sp);

    SAVE_PC();
    JSTaggedValue superCtor = SlowRuntimeStub::GetSuperConstructor(thread, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(superCtor);

    if (superCtor.IsJSFunction() && superCtor.IsConstructor() && !newTarget.IsUndefined()) {
        JSFunction *superCtorFunc = JSFunction::Cast(superCtor.GetTaggedObject());
        methodHandle.Update(superCtorFunc->GetMethod());
        if (superCtorFunc->IsBuiltinConstructor()) {
            ASSERT(methodHandle->GetNumVregsWithCallField() == 0);
            size_t frameSize =
                InterpretedFrame::NumOfMembers() + range + NUM_MANDATORY_JSFUNC_ARGS + 2; // 2:thread & numArgs
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            // copy args
            uint32_t index = 0;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(newSp);
            newSp[index++] = ToUintPtr(thread);
            newSp[index++] = range + NUM_MANDATORY_JSFUNC_ARGS;
            // func
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = superCtor.GetRawData();
            // newTarget
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = newTarget.GetRawData();
            // this
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            for (size_t i = 0; i < range; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(v0 + i);
            }

            InterpretedBuiltinFrame *state = GET_BUILTIN_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_BUILTIN_FRAME;
            state->pc = nullptr;
            state->function = superCtor;
            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime SuperCall ";
            JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                superCtorFunc->GetNativePointer())(ecmaRuntimeCallInfo);
            thread->SetCurrentSPFrame(sp);

            if (UNLIKELY(thread->HasPendingException())) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            LOG_INST() << "Exit: Runtime SuperCall ";
            SET_ACC(retValue);
            DISPATCH(WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8);
        }

        if (AssemblyIsFastNewFrameEnter(superCtorFunc, methodHandle)) {
            SAVE_PC();
            uint32_t numVregs = methodHandle->GetNumVregsWithCallField();
            uint32_t numDeclaredArgs = superCtorFunc->IsBase() ?
                methodHandle->GetNumArgsWithCallField() + 1 :  // +1 for this
                methodHandle->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
            // +1 for hidden this, explicit this may be overwritten after bc optimizer
            size_t frameSize = InterpretedFrame::NumOfMembers() + numVregs + numDeclaredArgs + 1;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(newSp) - 1;

            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            uint32_t index = 0;
            // initialize vregs value
            for (size_t i = 0; i < numVregs; ++i) {
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            // this
            JSTaggedValue thisObj;
            if (superCtorFunc->IsBase()) {
                thisObj = FastRuntimeStub::NewThisObject(thread, superCtor, newTarget, state);
                INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();
            } else {
                ASSERT(superCtorFunc->IsDerivedConstructor());
                newSp[index++] = newTarget.GetRawData();
                thisObj = JSTaggedValue::Undefined();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();

                state->function = superCtor;
                state->constpool = methodHandle->GetConstantPool();
                state->profileTypeInfo = superCtorFunc->GetProfileTypeInfo();
                state->env = superCtorFunc->GetLexicalEnv();
            }

            // the second condition ensure not push extra args
            for (size_t i = 0; i < range && index < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(v0 + i);
            }

            // set undefined to the extra prats of declare
            for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
            state->thisObj = thisObj;
            state->pc = pc = methodHandle->GetBytecodeArray();
            sp = newSp;
            state->acc = JSTaggedValue::Hole();

            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime SuperCall " << std::hex << reinterpret_cast<uintptr_t>(sp)
                                    << " " << std::hex << reinterpret_cast<uintptr_t>(pc);
            DISPATCH_OFFSET(0);
        }
    }

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::SuperCall(thread, thisFunc, newTarget, v0, range);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8);
}

void InterpreterAssembly::HandleWideSupercallthisrangePrefImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    JSMutableHandle<Method> methodHandle(thread, JSTaggedValue::Undefined());
    uint16_t range = READ_INST_16_1();
    uint16_t v0 = READ_INST_8_3();
    LOG_INST() << "intrinsics::supercall"
               << " range: " << range << " v" << v0;

    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue newTarget = GetNewTarget(sp);

    SAVE_PC();
    JSTaggedValue superCtor = SlowRuntimeStub::GetSuperConstructor(thread, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(superCtor);

    if (superCtor.IsJSFunction() && superCtor.IsConstructor() && !newTarget.IsUndefined()) {
        JSFunction *superCtorFunc = JSFunction::Cast(superCtor.GetTaggedObject());
        methodHandle.Update(superCtorFunc->GetMethod());
        if (superCtorFunc->IsBuiltinConstructor()) {
            ASSERT(methodHandle->GetNumVregsWithCallField() == 0);
            size_t frameSize =
                InterpretedFrame::NumOfMembers() + range + NUM_MANDATORY_JSFUNC_ARGS + 2; // 2:thread & numArgs
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            // copy args
            uint32_t index = 0;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(newSp);
            newSp[index++] = ToUintPtr(thread);
            newSp[index++] = range + NUM_MANDATORY_JSFUNC_ARGS;
            // func
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = superCtor.GetRawData();
            // newTarget
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = newTarget.GetRawData();
            // this
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            for (size_t i = 0; i < range; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(v0 + i);
            }

            InterpretedBuiltinFrame *state = GET_BUILTIN_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_BUILTIN_FRAME;
            state->pc = nullptr;
            state->function = superCtor;
            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime SuperCall ";
            JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                superCtorFunc->GetNativePointer())(ecmaRuntimeCallInfo);
            thread->SetCurrentSPFrame(sp);

            if (UNLIKELY(thread->HasPendingException())) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            LOG_INST() << "Exit: Runtime SuperCall ";
            SET_ACC(retValue);
            DISPATCH(WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8);
        }

        if (AssemblyIsFastNewFrameEnter(superCtorFunc, methodHandle)) {
            SAVE_PC();
            uint32_t numVregs = methodHandle->GetNumVregsWithCallField();
            uint32_t numDeclaredArgs = superCtorFunc->IsBase() ?
                methodHandle->GetNumArgsWithCallField() + 1 :  // +1 for this
                methodHandle->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
            // +1 for hidden this, explicit this may be overwritten after bc optimizer
            size_t frameSize = InterpretedFrame::NumOfMembers() + numVregs + numDeclaredArgs + 1;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(newSp) - 1;

            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            uint32_t index = 0;
            // initialize vregs value
            for (size_t i = 0; i < numVregs; ++i) {
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            // this
            JSTaggedValue thisObj;
            if (superCtorFunc->IsBase()) {
                thisObj = FastRuntimeStub::NewThisObject(thread, superCtor, newTarget, state);
                INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();
            } else {
                ASSERT(superCtorFunc->IsDerivedConstructor());
                newSp[index++] = newTarget.GetRawData();
                thisObj = JSTaggedValue::Undefined();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();

                state->function = superCtor;
                state->constpool = methodHandle->GetConstantPool();
                state->profileTypeInfo = superCtorFunc->GetProfileTypeInfo();
                state->env = superCtorFunc->GetLexicalEnv();
            }

            // the second condition ensure not push extra args
            for (size_t i = 0; i < range && index < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(v0 + i);
            }

            // set undefined to the extra prats of declare
            for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
            state->thisObj = thisObj;
            state->pc = pc = methodHandle->GetBytecodeArray();
            sp = newSp;
            state->acc = JSTaggedValue::Hole();

            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime SuperCall " << std::hex << reinterpret_cast<uintptr_t>(sp)
                                    << " " << std::hex << reinterpret_cast<uintptr_t>(pc);
            DISPATCH_OFFSET(0);
        }
    }

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::SuperCall(thread, thisFunc, newTarget, v0, range);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8);
}

void InterpreterAssembly::HandleWideCallthisrangePrefImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(WIDE_CALLTHISRANGE_PREF_IMM16_V8);
}

void InterpreterAssembly::HandleWideCallrangePrefImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(WIDE_CALLRANGE_PREF_IMM16_V8);
}

void InterpreterAssembly::HandleWideNewlexenvwithnamePrefImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t numVars = READ_INST_16_1();
    uint16_t scopeId = READ_INST_16_3();
    LOG_INST() << "intrinsics::newlexenvwithname"
               << " numVars " << numVars << " scopeId " << scopeId;

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewLexicalEnvWithName(thread, numVars, scopeId);
    INTERPRETER_RETURN_IF_ABRUPT(res);

    SET_ACC(res);
    (reinterpret_cast<InterpretedFrame *>(sp) - 1)->env = res;
    DISPATCH(WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16);
}

void InterpreterAssembly::HandleWideNewlexenvPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t numVars = READ_INST_16_1();
    LOG_INST() << "intrinsics::newlexenv"
               << " imm " << numVars;

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = FastRuntimeStub::NewLexicalEnv(thread, factory, numVars);
    if (res.IsHole()) {
        SAVE_PC();
        res = SlowRuntimeStub::NewLexicalEnv(thread, numVars);
        INTERPRETER_RETURN_IF_ABRUPT(res);
    }
    SET_ACC(res);
    (reinterpret_cast<InterpretedFrame *>(sp) - 1)->env = res;
    DISPATCH(WIDE_NEWLEXENV_PREF_IMM16);
}

void InterpreterAssembly::HandleWideNewobjrangePrefImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    JSMutableHandle<Method> methodHandle(thread, JSTaggedValue::Undefined());
    uint16_t numArgs = READ_INST_16_1();
    uint16_t firstArgRegIdx = READ_INST_8_3();
    LOG_INST() << "intrinsics::newobjRange " << numArgs << " v" << firstArgRegIdx;
    JSTaggedValue ctor = GET_VREG_VALUE(firstArgRegIdx);
    if (ctor.IsJSFunction() && ctor.IsConstructor()) {
        JSFunction *ctorFunc = JSFunction::Cast(ctor.GetTaggedObject());
        methodHandle.Update(ctorFunc->GetMethod());
        if (ctorFunc->IsBuiltinConstructor()) {
            ASSERT(methodHandle->GetNumVregsWithCallField() == 0);
            size_t frameSize = InterpretedFrame::NumOfMembers() + numArgs + 4;  // 3: this & numArgs & thread
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            // copy args
            uint32_t index = 0;
            // numArgs
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo*>(newSp);
            newSp[index++] = ToUintPtr(thread);
            newSp[index++] = numArgs + 2; // 2 : for newtarget / this
            // func
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = ctor.GetRawData();
            // newTarget
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = ctor.GetRawData();
            // this
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            for (size_t i = 1; i < numArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(firstArgRegIdx + i);
            }

            InterpretedBuiltinFrame *state = GET_BUILTIN_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_BUILTIN_FRAME;
            state->pc = nullptr;
            state->function = ctor;
            thread->SetCurrentSPFrame(newSp);

            LOG_INST() << "Entry: Runtime New.";
            SAVE_PC();
            JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                ctorFunc->GetNativePointer())(ecmaRuntimeCallInfo);
            thread->SetCurrentSPFrame(sp);
            if (UNLIKELY(thread->HasPendingException())) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            LOG_INST() << "Exit: Runtime New.";
            SET_ACC(retValue);
            DISPATCH(WIDE_NEWOBJRANGE_PREF_IMM16_V8);
        }

        if (AssemblyIsFastNewFrameEnter(ctorFunc, methodHandle)) {
            SAVE_PC();
            uint32_t numVregs = methodHandle->GetNumVregsWithCallField();
            uint32_t numDeclaredArgs = ctorFunc->IsBase() ?
                                       methodHandle->GetNumArgsWithCallField() + 1 :  // +1 for this
                                       methodHandle->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
            size_t frameSize = InterpretedFrame::NumOfMembers() + numVregs + numDeclaredArgs;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(newSp) - 1);

            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            uint32_t index = 0;
            // initialize vregs value
            for (size_t i = 0; i < numVregs; ++i) {
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            // this
            JSTaggedValue thisObj;
            if (ctorFunc->IsBase()) {
                thisObj = FastRuntimeStub::NewThisObject(thread, ctor, ctor, state);
                INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();
            } else {
                ASSERT(ctorFunc->IsDerivedConstructor());
                newSp[index++] = ctor.GetRawData();
                thisObj = JSTaggedValue::Undefined();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();

                state->function = ctor;
                state->constpool = methodHandle->GetConstantPool();
                state->profileTypeInfo = ctorFunc->GetProfileTypeInfo();
                state->env = ctorFunc->GetLexicalEnv();
            }

            // the second condition ensure not push extra args
            for (size_t i = 1; i < numArgs && index < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(firstArgRegIdx + i);
            }

            // set undefined to the extra prats of declare
            for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
            state->thisObj = thisObj;
            state->pc = pc = methodHandle->GetBytecodeArray();
            sp = newSp;
            state->acc = JSTaggedValue::Hole();

            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime New " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                                    << std::hex << reinterpret_cast<uintptr_t>(pc);
            DISPATCH_OFFSET(0);
        }
    }

    // bound function, proxy, other call types, enter slow path
    constexpr uint16_t firstArgOffset = 1;
    // Exclude func and newTarget
    uint16_t firstArgIdx = firstArgRegIdx + firstArgOffset;
    uint16_t length = numArgs - firstArgOffset;

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewObjRange(thread, ctor, ctor, firstArgIdx, length);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(WIDE_NEWOBJRANGE_PREF_IMM16_V8);
}

void InterpreterAssembly::HandleWideCreateobjectwithexcludedkeysPrefImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t numKeys = READ_INST_16_1();
    uint16_t v0 = READ_INST_8_3();
    uint16_t firstArgRegIdx = READ_INST_8_4();
    LOG_INST() << "intrinsics::createobjectwithexcludedkeys " << numKeys << " v" << firstArgRegIdx;

    JSTaggedValue obj = GET_VREG_VALUE(v0);

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CreateObjectWithExcludedKeys(thread, numKeys, obj, firstArgRegIdx);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedCreateobjecthavingmethodPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    LOG_INST() << "intrinsics::createobjecthavingmethod"
               << " imm:" << imm;
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSObject *result =
        JSObject::Cast(ConstantPool::GetMethodFromCache(thread, constpool, imm).GetTaggedObject());
    RESTORE_ACC();
    JSTaggedValue env = GET_ACC();

    SAVE_PC();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateObjectHavingMethod(thread, factory, result, env);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16);
}

void InterpreterAssembly::HandleDeprecatedLdhomeobjectPrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldhomeobject";

    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue homeObject = JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject();

    SET_ACC(homeObject);
    DISPATCH(DEPRECATED_LDHOMEOBJECT_PREF_NONE);
}

void InterpreterAssembly::HandleDeprecatedStclasstoglobalrecordPrefId32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_32_1();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    LOG_INST() << "intrinsics::stclasstoglobalrecord"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, false);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32);
}

void InterpreterAssembly::HandleDeprecatedStlettoglobalrecordPrefId32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_32_1();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    LOG_INST() << "intrinsics::stlettoglobalrecord"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, false);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32);
}

void InterpreterAssembly::HandleDeprecatedStconsttoglobalrecordPrefId32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_32_1();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    LOG_INST() << "intrinsics::stconsttoglobalrecord"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, true);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32);
}

void InterpreterAssembly::HandleDeprecatedLdmodulevarPrefId32Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    uint8_t innerFlag = READ_INST_8_5();

    constpool = GetConstantPool(sp);
    JSTaggedValue key = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    LOG_INST() << "intrinsics::ldmodulevar "
               << "string_id:" << stringId << ", "
               << "key: " << ConvertToString(EcmaString::Cast(key.GetTaggedObject()));

    JSTaggedValue moduleVar = SlowRuntimeStub::LdModuleVar(thread, key, innerFlag != 0);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8);
}
void InterpreterAssembly::HandleDeprecatedLdsuperbynamePrefId32V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t stringId = READ_INST_32_1();
    uint32_t v0 = READ_INST_8_5();
    JSTaggedValue obj = GET_VREG_VALUE(v0);
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);

    LOG_INST() << "intrinsics::ldsuperbyname"
               << "v" << v0 << " stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData();

    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, obj, propKey, thisFunc);

    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8);
}
void InterpreterAssembly::HandleDeprecatedLdobjbynamePrefId32V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_5();
    JSTaggedValue receiver = GET_VREG_VALUE(v0);

    uint16_t stringId = READ_INST_32_1();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    LOG_INST() << "intrinsics::ldobjbyname "
               << "v" << v0 << " stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << receiver.GetRawData();

    if (LIKELY(receiver.IsHeapObject())) {
        // fast path
        JSTaggedValue res = FastRuntimeStub::GetPropertyByName(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(DEPRECATED_LDOBJBYNAME_PREF_ID32_V8);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByName(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_LDOBJBYNAME_PREF_ID32_V8);
}

void InterpreterAssembly::HandleDeprecatedStmodulevarPrefId32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_32_1();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    auto key = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();

    LOG_INST() << "intrinsics::stmodulevar "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(key.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();

    SlowRuntimeStub::StModuleVar(thread, key, value);
    RESTORE_ACC();
    DISPATCH(DEPRECATED_STMODULEVAR_PREF_ID32);
}

void InterpreterAssembly::HandleDeprecatedGetmodulenamespacePrefId32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_32_1();
    constpool = GetConstantPool(sp);
    auto localName = ConstantPool::GetStringFromCache(thread, constpool, stringId);

    LOG_INST() << "intrinsics::getmodulenamespace "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(localName.GetTaggedObject()));

    JSTaggedValue moduleNamespace = SlowRuntimeStub::GetModuleNamespace(thread, localName);
    INTERPRETER_RETURN_IF_ABRUPT(moduleNamespace);
    SET_ACC(moduleNamespace);
    DISPATCH(DEPRECATED_GETMODULENAMESPACE_PREF_ID32);
}

void InterpreterAssembly::HandleDeprecatedStlexvarPrefImm16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_16_1();
    uint16_t slot = READ_INST_16_3();
    uint16_t v0 = READ_INST_8_5();
    LOG_INST() << "intrinsics::stlexvar"
               << " level:" << level << " slot:" << slot << " v" << v0;

    JSTaggedValue value = GET_VREG_VALUE(v0);
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue env = state->env;
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8);
}

void InterpreterAssembly::HandleDeprecatedStlexvarPrefImm8Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_8_1();
    uint16_t slot = READ_INST_8_2();
    uint16_t v0 = READ_INST_8_3();
    LOG_INST() << "intrinsics::stlexvar"
               << " level:" << level << " slot:" << slot << " v" << v0;

    JSTaggedValue value = GET_VREG_VALUE(v0);
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue env = state->env;
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8);
}

void InterpreterAssembly::HandleDeprecatedStlexvarPrefImm4Imm4V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_4_2();
    uint16_t slot = READ_INST_4_3();
    uint16_t v0 = READ_INST_8_2();
    LOG_INST() << "intrinsics::stlexvar"
               << " level:" << level << " slot:" << slot << " v" << v0;

    JSTaggedValue value = GET_VREG_VALUE(v0);
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue env = state->env;
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8);
}

void InterpreterAssembly::HandleDeprecatedAsyncfunctionrejectPrefV8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    uint16_t v2 = READ_INST_8_3();
    LOG_INST() << "intrinsics::asyncfunctionreject"
               << " v" << v0 << " v" << v1 << " v" << v2;

    JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_VREG_VALUE(v2);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncFunctionResolveOrReject(thread, asyncFuncObj, value, false);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedAsyncfunctionresolvePrefV8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    uint16_t v2 = READ_INST_8_3();
    LOG_INST() << "intrinsics::asyncfunctionresolve"
               << " v" << v0 << " v" << v1 << " v" << v2;

    JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_VREG_VALUE(v2);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncFunctionResolveOrReject(thread, asyncFuncObj, value, true);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedLdobjbyindexPrefV8Imm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint32_t idx = READ_INST_32_2();
    LOG_INST() << "intrinsics::ldobjbyindex"
               << " v" << v0 << " imm" << idx;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    // fast path
    if (LIKELY(receiver.IsHeapObject())) {
        JSTaggedValue res = FastRuntimeStub::GetPropertyByIndex(thread, receiver, idx);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByIndex(thread, receiver, idx, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32);
}

void InterpreterAssembly::HandleDeprecatedLdsuperbyvaluePrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint32_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::Ldsuperbyvalue"
               << " v" << v0 << " v" << v1;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_VREG_VALUE(v1);

    // slow path
    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, receiver, propKey, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedLdobjbyvaluePrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint32_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::Ldobjbyvalue"
               << " v" << v0 << " v" << v1;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_VREG_VALUE(v1);

    // fast path
    if (LIKELY(receiver.IsHeapObject())) {
        JSTaggedValue res = FastRuntimeStub::GetPropertyByValue(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(DEPRECATED_LDOBJBYVALUE_PREF_V8_V8);
        }
    }
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByValue(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_LDOBJBYVALUE_PREF_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedSetobjectwithprotoPrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::setobjectwithproto"
               << " v" << v0 << " v" << v1;
    JSTaggedValue proto = GET_VREG_VALUE(v0);
    JSTaggedValue obj = GET_VREG_VALUE(v1);
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::SetObjectWithProto(thread, proto, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedCopydatapropertiesPrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsic::copydataproperties"
               << " v" << v0 << " v" << v1;
    JSTaggedValue dst = GET_VREG_VALUE(v0);
    JSTaggedValue src = GET_VREG_VALUE(v1);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CopyDataProperties(thread, dst, src);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedAsyncfunctionawaituncaughtPrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::asyncfunctionawaituncaught"
               << " v" << v0 << " v" << v1;
    JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_VREG_VALUE(v1);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::AsyncFunctionAwaitUncaught(thread, asyncFuncObj, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedSuspendgeneratorPrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::suspendgenerator"
               << " v" << v0 << " v" << v1;
    JSTaggedValue genObj = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_VREG_VALUE(v1);
    // suspend will record bytecode offset
    SAVE_PC();
    SAVE_ACC();
    JSTaggedValue res = SlowRuntimeStub::SuspendGenerator(thread, genObj, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);

    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget();
    [[maybe_unused]] auto fistPC = method->GetBytecodeArray();
    UPDATE_HOTNESS_COUNTER(-(pc - fistPC));
    LOG_INST() << "Exit: SuspendGenerator " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                            << std::hex << reinterpret_cast<uintptr_t>(state->pc);
    sp = state->base.prev;
    ASSERT(sp != nullptr);
    InterpretedFrame *prevState = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    pc = prevState->pc;
    // entry frame
    if (FrameHandler::IsEntryFrame(pc)) {
        state->acc = acc;
        return;
    }

    thread->SetCurrentSPFrame(sp);

    size_t jumpSize = GetJumpSizeAfterCall(pc);
    DISPATCH_OFFSET(jumpSize);
}

void InterpreterAssembly::HandleDeprecatedDelobjpropPrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::delobjprop"
               << " v0" << v0 << " v1" << v1;

    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue prop = GET_VREG_VALUE(v1);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::DelObjProp(thread, obj, prop);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);

    DISPATCH(DEPRECATED_DELOBJPROP_PREF_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedGettemplateobjectPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsic::gettemplateobject"
               << " v" << v0;

    JSTaggedValue literal = GET_VREG_VALUE(v0);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetTemplateObject(thread, literal);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_GETTEMPLATEOBJECT_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedGetresumemodePrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::getresumemode";
    uint16_t vs = READ_INST_8_1();
    JSTaggedValue objVal = GET_VREG_VALUE(vs);
    if (objVal.IsAsyncGeneratorObject()) {
        JSAsyncGeneratorObject *obj = JSAsyncGeneratorObject::Cast(objVal.GetTaggedObject());
        SET_ACC(JSTaggedValue(static_cast<int>(obj->GetResumeMode())));
    } else {
        JSGeneratorObject *obj = JSGeneratorObject::Cast(objVal.GetTaggedObject());
        SET_ACC(JSTaggedValue(static_cast<int>(obj->GetResumeMode())));
    }
    DISPATCH(DEPRECATED_GETRESUMEMODE_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedResumegeneratorPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::resumegenerator";
    uint16_t vs = READ_INST_8_1();
    JSTaggedValue objVal = GET_VREG_VALUE(vs);
    if (objVal.IsAsyncGeneratorObject()) {
        JSAsyncGeneratorObject *obj = JSAsyncGeneratorObject::Cast(objVal.GetTaggedObject());
        SET_ACC(obj->GetResumeResult());
    } else {
        JSGeneratorObject *obj = JSGeneratorObject::Cast(objVal.GetTaggedObject());
        SET_ACC(obj->GetResumeResult());
    }
    DISPATCH(DEPRECATED_RESUMEGENERATOR_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_16_5();
    uint16_t v0 = READ_INST_8_7();
    uint16_t v1 = READ_INST_8_8();
    LOG_INST() << "intrinsics::defineclasswithbuffer"
               << " method id:" << methodId << " lexenv: v" << v0 << " parent: v" << v1;

    JSTaggedValue lexenv = GET_VREG_VALUE(v0);
    JSTaggedValue proto = GET_VREG_VALUE(v1);

    SAVE_PC();
    JSTaggedValue res =
        SlowRuntimeStub::CreateClassWithBuffer(thread, proto, lexenv, GetConstantPool(sp),
                                               methodId, methodId + 1, GetModule(sp),
                                               JSTaggedValue(length));

    INTERPRETER_RETURN_IF_ABRUPT(res);
    ASSERT(res.IsClassConstructor());

    SET_ACC(res);
    DISPATCH(DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedCallspreadPrefV8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    uint16_t v2 = READ_INST_8_3();
    LOG_INST() << "intrinsics::callspread"
                << " v" << v0 << " v" << v1 << " v" << v2;
    JSTaggedValue func = GET_VREG_VALUE(v0);
    JSTaggedValue obj = GET_VREG_VALUE(v1);
    JSTaggedValue array = GET_VREG_VALUE(v2);

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CallSpread(thread, func, obj, array);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);

    DISPATCH(DEPRECATED_CALLSPREAD_PREF_V8_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedCallargs3PrefV8V8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedCallargs2PrefV8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEPRECATED_CALLARGS2_PREF_V8_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedCallarg1PrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEPRECATED_CALLARG1_PREF_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedCallarg0PrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEPRECATED_CALLARG0_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedDecPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::dec"
               << " v" << v0;

    JSTaggedValue value = GET_VREG_VALUE(v0);
    // number, fast path
    if (value.IsInt()) {
        int32_t a0 = value.GetInt();
        if (UNLIKELY(a0 == INT32_MIN)) {
            auto ret = static_cast<double>(a0) - 1.0;
            SET_ACC(JSTaggedValue(ret));
        } else {
            SET_ACC(JSTaggedValue(a0 - 1));
        }
    } else if (value.IsDouble()) {
        SET_ACC(JSTaggedValue(value.GetDouble() - 1.0));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Dec(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(DEPRECATED_DEC_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedIncPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::inc"
               << " v" << v0;

    JSTaggedValue value = GET_VREG_VALUE(v0);
    // number fast path
    if (value.IsInt()) {
        int32_t a0 = value.GetInt();
        if (UNLIKELY(a0 == INT32_MAX)) {
            auto ret = static_cast<double>(a0) + 1.0;
            SET_ACC(JSTaggedValue(ret));
        } else {
            SET_ACC(JSTaggedValue(a0 + 1));
        }
    } else if (value.IsDouble()) {
        SET_ACC(JSTaggedValue(value.GetDouble() + 1.0));
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Inc(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(DEPRECATED_INC_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedNotPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::not"
               << " v" << v0;
    JSTaggedValue value = GET_VREG_VALUE(v0);
    int32_t number;
    // number, fast path
    if (value.IsInt()) {
        number = static_cast<int32_t>(value.GetInt());
        SET_ACC(JSTaggedValue(~number));  // NOLINT(hicpp-signed-bitwise);
    } else if (value.IsDouble()) {
        number = base::NumberHelper::DoubleToInt(value.GetDouble(), base::INT32_BITS);
        SET_ACC(JSTaggedValue(~number));  // NOLINT(hicpp-signed-bitwise);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Not(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(DEPRECATED_NOT_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedNegPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::neg"
               << " v" << v0;
    JSTaggedValue value = GET_VREG_VALUE(v0);
    // fast path
    if (value.IsInt()) {
        if (value.GetInt() == 0) {
            SET_ACC(JSTaggedValue(-0.0));
        } else {
            SET_ACC(JSTaggedValue(-value.GetInt()));
        }
    } else if (value.IsDouble()) {
        SET_ACC(JSTaggedValue(-value.GetDouble()));
    } else {  // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::Neg(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(DEPRECATED_NEG_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedTonumericPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::tonumeric"
               << " v" << v0;
    JSTaggedValue value = GET_VREG_VALUE(v0);
    if (value.IsNumber() || value.IsBigInt()) {
        // fast path
        SET_ACC(value);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::ToNumeric(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(DEPRECATED_TONUMERIC_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedCallthisrangePrefImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8);
}

void InterpreterAssembly::HandleDeprecatedTonumberPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::tonumber"
               << " v" << v0;
    JSTaggedValue value = GET_VREG_VALUE(v0);
    if (value.IsNumber()) {
        // fast path
        SET_ACC(value);
    } else {
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::ToNumber(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(DEPRECATED_TONUMBER_PREF_V8);
}

void InterpreterAssembly::HandleDeprecatedCreateobjectwithbufferPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    LOG_INST() << "intrinsics::createobjectwithbuffer"
               << " imm:" << imm;
    constpool = GetConstantPool(sp);
    JSObject *result =
        JSObject::Cast(ConstantPool::GetMethodFromCache(thread, constpool, imm).GetTaggedObject());

    SAVE_PC();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateObjectWithBuffer(thread, factory, result);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16);
}

void InterpreterAssembly::HandleDeprecatedCreatearraywithbufferPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    LOG_INST() << "intrinsics::createarraywithbuffer"
               << " imm:" << imm;
    constpool = GetConstantPool(sp);
    JSArray *result =
        JSArray::Cast(ConstantPool::GetMethodFromCache(thread, constpool, imm).GetTaggedObject());
    SAVE_PC();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateArrayWithBuffer(thread, factory, result);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16);
}

void InterpreterAssembly::HandleDeprecatedGetiteratornextPrefV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    uint16_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsic::getiteratornext"
               << " v" << v0 << " v" << v1;
    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue method = GET_VREG_VALUE(v1);
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::GetIteratorNext(thread, obj, method);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_GETITERATORNEXT_PREF_V8_V8);
}

void InterpreterAssembly::HandleDeprecatedPoplexenvPrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue currentLexenv = state->env;
    JSTaggedValue parentLexenv = LexicalEnv::Cast(currentLexenv.GetTaggedObject())->GetParentEnv();
    (reinterpret_cast<InterpretedFrame *>(sp) - 1)->env = parentLexenv;
    DISPATCH(DEPRECATED_POPLEXENV_PREF_NONE);
}

void InterpreterAssembly::HandleDeprecatedLdlexenvPrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldlexenv ";
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue currentLexenv = state->env;
    SET_ACC(currentLexenv);
    DISPATCH(DEPRECATED_LDLEXENV_PREF_NONE);
}

void InterpreterAssembly::HandleCallRuntime(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
}

void InterpreterAssembly::HandleWide(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
}

void InterpreterAssembly::HandleDeprecated(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
}

void InterpreterAssembly::HandleJnstricteqV8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNSTRICTEQ_V8_IMM16);
}

void InterpreterAssembly::HandleJnstricteqV8Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNSTRICTEQ_V8_IMM8);
}

void InterpreterAssembly::HandleJstricteqV8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JSTRICTEQ_V8_IMM16);
}

void InterpreterAssembly::HandleJstricteqV8Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JSTRICTEQ_V8_IMM8);
}

void InterpreterAssembly::HandleJneV8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNE_V8_IMM16);
}

void InterpreterAssembly::HandleJneV8Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNE_V8_IMM8);
}

void InterpreterAssembly::HandleJeqV8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JEQ_V8_IMM16);
}

void InterpreterAssembly::HandleJeqV8Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNE_V8_IMM8);
}

void InterpreterAssembly::HandleJnstrictequndefinedImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNSTRICTEQUNDEFINED_IMM16);
}

void InterpreterAssembly::HandleJnstrictequndefinedImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNSTRICTEQUNDEFINED_IMM8);
}

void InterpreterAssembly::HandleJstrictequndefinedImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JSTRICTEQUNDEFINED_IMM16);
}

void InterpreterAssembly::HandleJstrictequndefinedImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JSTRICTEQUNDEFINED_IMM8);
}

void InterpreterAssembly::HandleJneundefinedImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNEUNDEFINED_IMM16);
}

void InterpreterAssembly::HandleJneundefinedImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNEUNDEFINED_IMM8);
}

void InterpreterAssembly::HandleJequndefinedImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JEQUNDEFINED_IMM16);
}

void InterpreterAssembly::HandleJequndefinedImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JEQUNDEFINED_IMM8);
}

void InterpreterAssembly::HandleJnstricteqnullImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNSTRICTEQNULL_IMM16);
}

void InterpreterAssembly::HandleJnstricteqnullImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNSTRICTEQNULL_IMM8);
}

void InterpreterAssembly::HandleCallarg1Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLARG1_IMM8_V8);
}

void InterpreterAssembly::HandleJstricteqnullImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JSTRICTEQNULL_IMM16);
}

void InterpreterAssembly::HandleJstricteqnullImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JSTRICTEQNULL_IMM8);
}

void InterpreterAssembly::HandleJnenullImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNENULL_IMM16);
}

void InterpreterAssembly::HandleJnenullImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNENULL_IMM8);
}

void InterpreterAssembly::HandleStownbynamewithnamesetImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    uint32_t v0 = READ_INST_8_4();
    constpool = GetConstantPool(sp);
    LOG_INST() << "intrinsics::stownbynamewithnameset "
               << "v" << v0 << " stringId:" << stringId;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsJSObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
        JSTaggedValue value = GET_ACC();
        // fast path
        SAVE_ACC();
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            JSFunction::SetFunctionNameNoPrefix(thread, JSFunction::Cast(value.GetTaggedObject()), propKey);
            RESTORE_ACC();
            DISPATCH(STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8);
        }
        RESTORE_ACC();
    }

    SAVE_ACC();
    SAVE_PC();
    receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
    auto propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);  // Maybe moved by GC
    auto value = GET_ACC();                                  // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StOwnByNameWithNameSet(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleStownbyvaluewithnamesetImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_2();
    uint32_t v1 = READ_INST_8_3();
    LOG_INST() << "intrinsics::stownbyvaluewithnameset"
               << " v" << v0 << " v" << v1;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, propKey, value);

        // SetPropertyByValue maybe gc need update the value
        RESTORE_ACC();
        propKey = GET_VREG_VALUE(v1);
        value = GET_ACC();
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            JSFunction::SetFunctionNameNoPrefix(thread, JSFunction::Cast(value.GetTaggedObject()), propKey);
            RESTORE_ACC();
            DISPATCH(STOWNBYVALUEWITHNAMESET_IMM16_V8_V8);
        }
    }

    // slow path
    SAVE_ACC();
    SAVE_PC();
    receiver = GET_VREG_VALUE(v0);      // Maybe moved by GC
    auto propKey = GET_VREG_VALUE(v1);  // Maybe moved by GC
    auto value = GET_ACC();             // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StOwnByValueWithNameSet(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYVALUEWITHNAMESET_IMM16_V8_V8);
}

void InterpreterAssembly::HandleJeqnullImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JEQNULL_IMM16);
}

void InterpreterAssembly::HandleJeqnullImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JEQNULL_IMM8);
}

void InterpreterAssembly::HandleJnstricteqzImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNSTRICTEQZ_IMM16);
}

void InterpreterAssembly::HandleJnstricteqzImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JNSTRICTEQZ_IMM8);
}

void InterpreterAssembly::HandleSttoglobalrecordImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    LOG_INST() << "intrinsics::stconsttoglobalrecord"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, true);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STCONSTTOGLOBALRECORD_IMM16_ID16);
}

void InterpreterAssembly::HandleStconsttoglobalrecordImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    JSTaggedValue constantPool = state->constpool;
    JSTaggedValue propKey = ConstantPool::Cast(constantPool.GetTaggedObject())
        ->GetObjectFromCache(stringId);
    LOG_INST() << "intrinsics::stconsttoglobalrecord"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, true);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STCONSTTOGLOBALRECORD_IMM16_ID16);
}

void InterpreterAssembly::HandleLdlocalmodulevarImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_8_0();

    LOG_INST() << "intrinsics::ldmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdLocalModuleVar(thread, index);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(LDLOCALMODULEVAR_IMM8);
}

void InterpreterAssembly::HandleStsuperbynameImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    uint32_t v0 = READ_INST_8_4();
    constpool = GetConstantPool(sp);

    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    JSTaggedValue value = GET_ACC();

    LOG_INST() << "intrinsics::stsuperbyname"
               << "v" << v0 << " stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData()
               << ", value:" << value.GetRawData();

    // slow path
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::StSuperByValue(thread, obj, propKey, value, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STSUPERBYNAME_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleLdsuperbynameImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    JSTaggedValue obj = GET_ACC();

    LOG_INST() << "intrinsics::ldsuperbyname stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData();

    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, obj, propKey, thisFunc);

    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDSUPERBYNAME_IMM16_ID16);
}

void InterpreterAssembly::HandleLdsuperbynameImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();

    JSTaggedValue obj = GET_ACC();
    LOG_INST() << "intrinsics::ldsuperbyname stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData();

    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, obj, propKey, thisFunc);

    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDSUPERBYNAME_IMM8_ID16);
}

void InterpreterAssembly::HandleStownbynameImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    uint32_t v0 = READ_INST_8_4();
    LOG_INST() << "intrinsics::stownbyname "
               << "v" << v0 << " stringId:" << stringId;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsJSObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        constpool = GetConstantPool(sp);
        JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
        RESTORE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        SAVE_ACC();
        receiver = GET_VREG_VALUE(v0);
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName<ObjectFastOperator::Status::UseOwn>
                            (thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOWNBYNAME_IMM16_ID16_V8);
        }
        RESTORE_ACC();
    }

    SAVE_ACC();
    constpool = GetConstantPool(sp);
    auto propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);  // Maybe moved by GC
    RESTORE_ACC();
    auto value = GET_ACC();                                  // Maybe moved by GC
    receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StOwnByName(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYNAME_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleStobjbynameImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    uint32_t v0 = READ_INST_8_4();
#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_ACC();
        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByName(thread, receiver, firstValue, secondValue, value);
        }
        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYNAME_IMM16_ID16_V8);
        } else if (!firstValue.IsHole()) { // IC miss and not enter the megamorphic state, store as polymorphic
            SAVE_ACC();
            constpool = GetConstantPool(sp);
            JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
            RESTORE_ACC();
            value = GET_ACC();
            receiver = GET_VREG_VALUE(v0);
            profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
            res = ICRuntimeStub::StoreICByName(thread,
                                               profileTypeArray,
                                               receiver, propKey, value, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYNAME_IMM16_ID16_V8);
        }
    }
#endif
    LOG_INST() << "intrinsics::stobjbyname "
               << "v" << v0 << " stringId:" << stringId;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        constpool = GetConstantPool(sp);
        JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
        RESTORE_ACC();
        JSTaggedValue value = GET_ACC();
        receiver = GET_VREG_VALUE(v0);
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYNAME_IMM16_ID16_V8);
        }
        RESTORE_ACC();
    }
    // slow path
    SAVE_ACC();
    SAVE_PC();
    constpool = GetConstantPool(sp);                    // Maybe moved by GC
    auto propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);  // Maybe moved by GC
    RESTORE_ACC();
    JSTaggedValue value = GET_ACC();                                  // Maybe moved by GC
    receiver = GET_VREG_VALUE(v0);
    JSTaggedValue res = SlowRuntimeStub::StObjByName(thread, receiver, propKey, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOBJBYNAME_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleLdobjbynameImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
#if ECMASCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        JSTaggedValue receiver = GET_ACC();
        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByName(thread, receiver, firstValue, secondValue);
        }
        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYNAME_IMM16_ID16);
        } else if (!firstValue.IsHole()) { // IC miss and not enter the megamorphic state, store as polymorphic
            uint16_t stringId = READ_INST_16_2();
            SAVE_ACC();
            constpool = GetConstantPool(sp);
            JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
            RESTORE_ACC();
            receiver = GET_ACC();
            profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
            res = ICRuntimeStub::LoadICByName(thread,
                                              profileTypeArray,
                                              receiver, propKey, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYNAME_IMM16_ID16);
        }
    }
#endif
    uint16_t stringId = READ_INST_16_2();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    JSTaggedValue receiver = GET_ACC();
    LOG_INST() << "intrinsics::ldobjbyname stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << receiver.GetRawData();

    if (LIKELY(receiver.IsHeapObject())) {
        // fast path
        JSTaggedValue res = FastRuntimeStub::GetPropertyByName(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYNAME_IMM16_ID16);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByName(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYNAME_IMM16_ID16);
}

void InterpreterAssembly::HandleLdobjbynameImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
#if ECMASCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        JSTaggedValue receiver = GET_ACC();
        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByName(thread, receiver, firstValue, secondValue);
        }
        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYNAME_IMM8_ID16);
        } else if (!firstValue.IsHole()) { // IC miss and not enter the megamorphic state, store as polymorphic
            uint16_t stringId = READ_INST_16_1();
            SAVE_ACC();
            constpool = GetConstantPool(sp);
            JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
            RESTORE_ACC();
            receiver = GET_ACC();
            profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
            res = ICRuntimeStub::LoadICByName(thread,
                                              profileTypeArray,
                                              receiver, propKey, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYNAME_IMM8_ID16);
        }
    }
#endif
    uint16_t stringId = READ_INST_16_1();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    RESTORE_ACC();
    JSTaggedValue receiver = GET_ACC();
    LOG_INST() << "intrinsics::ldobjbyname stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << receiver.GetRawData();

    if (LIKELY(receiver.IsHeapObject())) {
        // fast path
        JSTaggedValue res = FastRuntimeStub::GetPropertyByName(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYNAME_IMM8_ID16);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByName(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYNAME_IMM8_ID16);
}

void InterpreterAssembly::HandleTrystglobalbynameImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::GetStringFromCache(thread, constpool, stringId);

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();

    LOG_INST() << "intrinsics::trystglobalbyname"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        JSTaggedValue res = ICRuntimeStub::StoreGlobalICByName(thread,
                                                               ProfileTypeInfo::Cast(
                                                                   tmpProfileTypeInfo.GetTaggedObject()),
                                                               globalObj, propKey, value, slotId, true);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(TRYSTGLOBALBYNAME_IMM16_ID16);
    }
#endif

    auto recordResult = SlowRuntimeStub::LdGlobalRecord(thread, propKey);
    SAVE_PC();
    // 1. find from global record
    if (!recordResult.IsUndefined()) {
        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        JSTaggedValue res = SlowRuntimeStub::TryUpdateGlobalRecord(thread, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    } else {
        // 2. find from global object
        auto globalResult = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, propKey);
        if (globalResult.IsHole()) {
            auto result = SlowRuntimeStub::ThrowReferenceError(thread, propKey, " is not defined");
            INTERPRETER_RETURN_IF_ABRUPT(result);
        }
        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        JSTaggedValue res = SlowRuntimeStub::StGlobalVar(thread, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    }
    DISPATCH(TRYSTGLOBALBYNAME_IMM16_ID16);
}

void InterpreterAssembly::HandleTryldglobalbynameImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    constpool = GetConstantPool(sp);
    auto prop = ConstantPool::GetStringFromCache(thread, constpool, stringId);

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();

    LOG_INST() << "intrinsics::tryldglobalbyname "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(prop.GetTaggedObject()));

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        JSTaggedValue res = ICRuntimeStub::LoadGlobalICByName(thread,
                                                              ProfileTypeInfo::Cast(
                                                                  tmpProfileTypeInfo.GetTaggedObject()),
                                                              globalObj, prop, slotId, true);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(TRYLDGLOBALBYNAME_IMM16_ID16);
    }
#endif

    // order: 1. global record 2. global object
    JSTaggedValue result = SlowRuntimeStub::LdGlobalRecord(thread, prop);
    if (!result.IsUndefined()) {
        SET_ACC(PropertyBox::Cast(result.GetTaggedObject())->GetValue());
    } else {
        JSTaggedValue globalResult = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, prop);
        if (!globalResult.IsHole()) {
            SET_ACC(globalResult);
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::TryLdGlobalByNameFromGlobalProto(thread, globalObj, prop);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
    }

    DISPATCH(TRYLDGLOBALBYNAME_IMM16_ID16);
}

void InterpreterAssembly::HandleStmodulevarImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_8_0();

    LOG_INST() << "intrinsics::stmodulevar index:" << index;

    JSTaggedValue value = GET_ACC();

    SlowRuntimeStub::StModuleVar(thread, index, value);
    RESTORE_ACC();
    DISPATCH(STMODULEVAR_IMM8);
}

void InterpreterAssembly::HandleGetmodulenamespaceImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_8_0();

    LOG_INST() << "intrinsics::getmodulenamespace index:" << index;

    JSTaggedValue moduleNamespace = SlowRuntimeStub::GetModuleNamespace(thread, index);
    INTERPRETER_RETURN_IF_ABRUPT(moduleNamespace);
    SET_ACC(moduleNamespace);
    DISPATCH(GETMODULENAMESPACE_IMM8);
}

void InterpreterAssembly::HandleLdobjbyindexImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t idx = READ_INST_32_1();
    LOG_INST() << "intrinsics::ldobjbyindex"
               << " imm" << idx;

    JSTaggedValue receiver = GET_ACC();
    // fast path
    if (LIKELY(receiver.IsHeapObject())) {
        JSTaggedValue res = FastRuntimeStub::GetPropertyByIndex(thread, receiver, idx);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYINDEX_IMM16_IMM16);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByIndex(thread, receiver, idx, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYINDEX_IMM16_IMM16);
}

void InterpreterAssembly::HandleLdobjbyindexImm8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t idx = READ_INST_16_1();
    LOG_INST() << "intrinsics::ldobjbyindex"
               << " imm" << idx;

    JSTaggedValue receiver = GET_ACC();
    // fast path
    if (LIKELY(receiver.IsHeapObject())) {
        JSTaggedValue res = FastRuntimeStub::GetPropertyByIndex(thread, receiver, idx);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYINDEX_IMM8_IMM16);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByIndex(thread, receiver, idx, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYINDEX_IMM8_IMM16);
}

void InterpreterAssembly::HandleStsuperbyvalueImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_2();
    uint32_t v1 = READ_INST_8_3();

    LOG_INST() << "intrinsics::stsuperbyvalue"
               << " v" << v0 << " v" << v1;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_VREG_VALUE(v1);
    JSTaggedValue value = GET_ACC();

    // slow path
    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::StSuperByValue(thread, receiver, propKey, value, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STSUPERBYVALUE_IMM16_V8_V8);
}

void InterpreterAssembly::HandleLdsuperbyvalueImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_2();
    LOG_INST() << "intrinsics::Ldsuperbyvalue"
               << " v" << v0;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_ACC();

    // slow path
    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, receiver, propKey, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDSUPERBYVALUE_IMM16_V8);
}

void InterpreterAssembly::HandleLdsuperbyvalueImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::Ldsuperbyvalue"
               << " v" << v0;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_ACC();

    // slow path
    SAVE_PC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, receiver, propKey, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDSUPERBYVALUE_IMM8_V8);
}

void InterpreterAssembly::HandleLdobjbyvalueImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_2();
    LOG_INST() << "intrinsics::Ldobjbyvalue"
               << " v" << v0;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_ACC();

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profiltmpProfileTypeInfoeTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByValue(thread, receiver, propKey, firstValue, secondValue);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::LoadICByValue(thread,
                                               profileTypeArray,
                                               receiver, propKey, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYVALUE_IMM16_V8);
        }
    }
#endif
    // fast path
    if (LIKELY(receiver.IsHeapObject())) {
        JSTaggedValue res = FastRuntimeStub::GetPropertyByValue(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYVALUE_IMM16_V8);
        }
    }
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByValue(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYVALUE_IMM16_V8);
}

void InterpreterAssembly::HandleLdobjbyvalueImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::Ldobjbyvalue"
               << " v" << v0;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_ACC();

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByValue(thread, receiver, propKey, firstValue, secondValue);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::LoadICByValue(thread,
                                               profileTypeArray,
                                               receiver, propKey, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYVALUE_IMM8_V8);
        }
    }
#endif
    // fast path
    if (LIKELY(receiver.IsHeapObject())) {
        JSTaggedValue res = FastRuntimeStub::GetPropertyByValue(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYVALUE_IMM8_V8);
        }
    }
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByValue(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYVALUE_IMM8_V8);
}

void InterpreterAssembly::HandleJstricteqzImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JSTRICTEQZ_IMM16);
}

void InterpreterAssembly::HandleJstricteqzImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(JSTRICTEQZ_IMM8);
}

void InterpreterAssembly::HandleDefineclasswithbufferImm16Id16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_2();
    uint16_t literaId = READ_INST_16(6);
    uint16_t length = READ_INST_16(8);
    uint16_t v0 = READ_INST_8_8();
    LOG_INST() << "intrinsics::defineclasswithbuffer"
               << " method id:" << methodId << " lexenv: v" << v0;

    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue proto = GET_VREG_VALUE(v0);

    SAVE_PC();
    JSTaggedValue res =
        SlowRuntimeStub::CreateClassWithBuffer(thread, proto, state->env, GetConstantPool(sp),
                                               methodId, literaId, GetModule(sp), JSTaggedValue(length));

    INTERPRETER_RETURN_IF_ABRUPT(res);
    ASSERT(res.IsClassConstructor());

    SET_ACC(res);
    DISPATCH(DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleDefineclasswithbufferImm8Id16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t literaId = READ_INST_16_3();
    uint16_t length = READ_INST_16_5();
    uint16_t v0 = READ_INST_8_7();
    LOG_INST() << "intrinsics::defineclasswithbuffer"
               << " method id:" << methodId << " lexenv: v" << v0;

    JSTaggedValue proto = GET_VREG_VALUE(v0);

    SAVE_PC();
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue res =
        SlowRuntimeStub::CreateClassWithBuffer(thread, proto, state->env, GetConstantPool(sp),
                                               methodId, literaId, GetModule(sp), JSTaggedValue(length));

    INTERPRETER_RETURN_IF_ABRUPT(res);
    ASSERT(res.IsClassConstructor());

    SET_ACC(res);
    DISPATCH(DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleWideLdpatchvarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t index = READ_INST_16_1();
    LOG_INST() << "intrinsics::ldpatchvar" << " imm: " << index;

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdPatchVar(thread, index);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(WIDE_LDPATCHVAR_PREF_IMM16);
}

void InterpreterAssembly::HandleWideStpatchvarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t index = READ_INST_16_1();
    LOG_INST() << "intrinsics::stpatchvar" << " imm: " << index;
    JSTaggedValue value = GET_ACC();

    SAVE_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::StPatchVar(thread, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(WIDE_STPATCHVAR_PREF_IMM16);
}

void InterpreterAssembly::HandleLdPrivatePropertyImm8Imm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(LDPRIVATEPROPERTY_IMM8_IMM16_IMM16);
}

void InterpreterAssembly::HandleStPrivatePropertyImm8Imm16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8);
}

void InterpreterAssembly::HandleTestInImm8Imm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(TESTIN_IMM8_IMM16_IMM16);
}

void InterpreterAssembly::HandleCallRuntimeNotifyConcurrentResultPrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE);
}

void InterpreterAssembly::HandleDefineFieldByNameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINEFIELDBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleDefinePropertyByNameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINEPROPERTYBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleCallRuntimeDefineFieldByValuePrefImm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8);
}

void InterpreterAssembly::HandleCallRuntimeDefineFieldByIndexPrefImm8Imm32V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8);
}

void InterpreterAssembly::HandleCallRuntimeToPropertyKeyPrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLRUNTIME_TOPROPERTYKEY_PREF_NONE);
}

void InterpreterAssembly::HandleCallRuntimeCreatePrivatePropertyPrefImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16);
}

void InterpreterAssembly::HandleCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8);
}

void InterpreterAssembly::HandleCallRuntimeCallInitPrefImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLRUNTIME_CALLINIT_PREF_IMM8_V8);
}

void InterpreterAssembly::HandleCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_3();
    uint16_t literaId = READ_INST_16_5();
    uint16_t length = READ_INST_16_7();
    uint16_t v0 = READ_INST_8_9();
    LOG_INST() << "intrinsics::definesendableclass"
                << " method id:" << methodId << " base: v" << v0;

    JSTaggedValue base = GET_VREG_VALUE(v0);

    SAVE_PC();
    JSTaggedValue res =
        SlowRuntimeStub::CreateSharedClass(thread, base, GetConstantPool(sp), methodId, literaId,
                                           length, GetModule(sp));

    INTERPRETER_RETURN_IF_ABRUPT(res);
    ASSERT(res.IsClassConstructor());
    ASSERT(res.IsJSSharedFunction());
    SET_ACC(res);
    DISPATCH(CALLRUNTIME_DEFINESENDABLECLASS_PREF_IMM16_ID16_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleCallRuntimeLdSendableClassPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_16_1();
    LOG_INST() << "intrinsics::LdSendableClass level: " << level;
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    auto res = SlowRuntimeStub::LdSendableClass(thread, state->env, level);
    ASSERT(res.IsJSSharedFunction());
    SET_ACC(res);
    DISPATCH(CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16);
}

void InterpreterAssembly::HandleStthisbyvalueImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_2();

    LOG_INST() << "intrinsics::stthisbyvalue"
               << " v" << v0;

    JSTaggedValue receiver = GetThis(sp);
#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue propKey = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_ACC();
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByValue(thread, receiver, propKey, firstValue, secondValue, value);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::StoreICByValue(thread,
                                                profileTypeArray,
                                                receiver, propKey, value, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STTHISBYVALUE_IMM16_V8);
        }
    }
#endif
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        JSTaggedValue propKey = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STTHISBYVALUE_IMM16_V8);
        }
        RESTORE_ACC();
    }
    {
        // slow path
        SAVE_ACC();
        receiver = GetThis(sp);                       // Maybe moved by GC
        JSTaggedValue propKey = GET_VREG_VALUE(v0);   // Maybe moved by GC
        JSTaggedValue value = GET_ACC();              // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StObjByValue(thread, receiver, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    }
    DISPATCH(STTHISBYVALUE_IMM16_V8);
}

void InterpreterAssembly::HandleStthisbyvalueImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::stthisbyvalue"
               << " v" << v0;

    JSTaggedValue receiver = GetThis(sp);
#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue propKey = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_ACC();
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByValue(thread, receiver, propKey, firstValue, secondValue, value);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::StoreICByValue(thread,
                                                profileTypeArray,
                                                receiver, propKey, value, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STTHISBYVALUE_IMM8_V8);
        }
    }
#endif
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        JSTaggedValue propKey = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STTHISBYVALUE_IMM8_V8);
        }
        RESTORE_ACC();
    }
    {
        // slow path
        SAVE_ACC();
        receiver = GetThis(sp);                       // Maybe moved by GC
        JSTaggedValue propKey = GET_VREG_VALUE(v0);   // Maybe moved by GC
        JSTaggedValue value = GET_ACC();              // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StObjByValue(thread, receiver, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    }
    DISPATCH(STTHISBYVALUE_IMM8_V8);
}

void InterpreterAssembly::HandleLdthisbyvalueImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::Ldthisbyvalue";

    JSTaggedValue receiver = GetThis(sp);
    JSTaggedValue propKey = GET_ACC();

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByValue(thread, receiver, propKey, firstValue, secondValue);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::LoadICByValue(thread,
                                               profileTypeArray,
                                               receiver, propKey, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYVALUE_IMM16);
        }
    }
#endif
    if (LIKELY(receiver.IsHeapObject())) {
        // fast path
        JSTaggedValue res = FastRuntimeStub::GetPropertyByValue(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYVALUE_IMM16);
        }
    }
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByValue(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDTHISBYVALUE_IMM16);
}

void InterpreterAssembly::HandleLdthisbyvalueImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::Ldthisbyvalue";

    JSTaggedValue receiver = GetThis(sp);
    JSTaggedValue propKey = GET_ACC();

#if ECMSCRIPT_ENABLE_IC
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    auto tmpProfileTypeInfo = state->profileTypeInfo;
    if (!tmpProfileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(tmpProfileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByValue(thread, receiver, propKey, firstValue, secondValue);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::LoadICByValue(thread,
                                               profileTypeArray,
                                               receiver, propKey, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYVALUE_IMM8);
        }
    }
#endif
    // fast path
    if (LIKELY(receiver.IsHeapObject())) {
        JSTaggedValue res = FastRuntimeStub::GetPropertyByValue(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYVALUE_IMM8);
        }
    }
    // slow path
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByValue(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDTHISBYVALUE_IMM8);
}

void InterpreterAssembly::HandleStthisbynameImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        JSTaggedValue receiver = GetThis(sp);
        JSTaggedValue value = GET_ACC();
        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByName(thread, receiver, firstValue, secondValue, value);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            uint16_t stringId = READ_INST_16_2();
            constpool = GetConstantPool(sp);
            JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
            RESTORE_ACC();
            value = GET_ACC();
            receiver = GetThis(sp);
            profileTypeArray = ProfileTypeInfo::Cast(GetProfileTypeInfo(sp).GetTaggedObject());
            res = ICRuntimeStub::StoreICByName(thread, profileTypeArray, receiver, propKey, value, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STTHISBYNAME_IMM16_ID16);
        }
        RESTORE_ACC();
    }
#endif
    uint16_t stringId = READ_INST_16_2();
    LOG_INST() << "intrinsics::stthisbyname "
               << " stringId:" << stringId;
    JSTaggedValue receiver = GetThis(sp);
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        constpool = GetConstantPool(sp);
        JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
        RESTORE_ACC();
        JSTaggedValue value = GET_ACC();
        receiver = GetThis(sp);
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STTHISBYNAME_IMM16_ID16);
        }
        RESTORE_ACC();
    }
    // slow path
    SAVE_ACC();
    SAVE_PC();
    constpool = GetConstantPool(sp);
    auto propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);  // Maybe moved by GC
    RESTORE_ACC();
    JSTaggedValue value = GET_ACC();                  // Maybe moved by GC
    receiver = GetThis(sp);                           // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StObjByName(thread, receiver, propKey, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STTHISBYNAME_IMM16_ID16);
}

void InterpreterAssembly::HandleStthisbynameImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        JSTaggedValue receiver = GetThis(sp);
        JSTaggedValue value = GET_ACC();
        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByName(thread, receiver, firstValue, secondValue, value);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            uint16_t stringId = READ_INST_16_1();
            constpool = GetConstantPool(sp);
            JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
            RESTORE_ACC();
            value = GET_ACC();
            receiver = GetThis(sp);
            profileTypeArray = ProfileTypeInfo::Cast(GetProfileTypeInfo(sp).GetTaggedObject());
            res = ICRuntimeStub::StoreICByName(thread, profileTypeArray, receiver, propKey, value, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STTHISBYNAME_IMM8_ID16);
        }
        RESTORE_ACC();
    }
#endif
    uint16_t stringId = READ_INST_16_1();
    LOG_INST() << "intrinsics::stthisbyname "
               << " stringId:" << stringId;
    JSTaggedValue receiver = GetThis(sp);
    if (receiver.IsHeapObject()) {
        SAVE_ACC();
        constpool = GetConstantPool(sp);
        JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
        RESTORE_ACC();
        JSTaggedValue value = GET_ACC();
        receiver = GetThis(sp);
        // fast path
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STTHISBYNAME_IMM8_ID16);
        }
        RESTORE_ACC();
    }
    // slow path
    SAVE_ACC();
    SAVE_PC();
    constpool = GetConstantPool(sp);
    auto propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);  // Maybe moved by GC
    RESTORE_ACC();
    JSTaggedValue value = GET_ACC();                  // Maybe moved by GC
    receiver = GetThis(sp);                           // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StObjByName(thread, receiver, propKey, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STTHISBYNAME_IMM8_ID16);
}

void InterpreterAssembly::HandleLdthisbynameImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_16_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        JSTaggedValue receiver = GetThis(sp);
        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByName(thread, receiver, firstValue, secondValue);
        }
        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYNAME_IMM16_ID16);
        } else if (!firstValue.IsHole()) { // IC miss and not enter the megamorphic state, store as polymorphic
            uint16_t stringId = READ_INST_16_2();
            constpool = GetConstantPool(sp);
            JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
            receiver = GetThis(sp);
            profileTypeArray = ProfileTypeInfo::Cast(GetProfileTypeInfo(sp).GetTaggedObject());
            res = ICRuntimeStub::LoadICByName(thread,
                                              profileTypeArray,
                                              receiver, propKey, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYNAME_IMM16_ID16);
        }
    }
#endif
    uint16_t stringId = READ_INST_16_2();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    JSTaggedValue receiver = GetThis(sp);
    LOG_INST() << "intrinsics::ldthisbyname stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << receiver.GetRawData();

    if (LIKELY(receiver.IsHeapObject())) {
        // fast path
        JSTaggedValue res = FastRuntimeStub::GetPropertyByName(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYNAME_IMM16_ID16);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByName(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDTHISBYNAME_IMM16_ID16);
}

void InterpreterAssembly::HandleLdthisbynameImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        JSTaggedValue receiver = GetThis(sp);
        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByName(thread, receiver, firstValue, secondValue);
        }
        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYNAME_IMM8_ID16);
        } else if (!firstValue.IsHole()) { // IC miss and not enter the megamorphic state, store as polymorphic
            uint16_t stringId = READ_INST_16_1();
            constpool = GetConstantPool(sp);
            JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
            receiver = GetThis(sp);
            profileTypeArray = ProfileTypeInfo::Cast(GetProfileTypeInfo(sp).GetTaggedObject());
            res = ICRuntimeStub::LoadICByName(thread,
                                              profileTypeArray,
                                              receiver, propKey, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYNAME_IMM8_ID16);
        }
    }
#endif
    uint16_t stringId = READ_INST_16_1();
    constpool = GetConstantPool(sp);
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    JSTaggedValue receiver = GetThis(sp);
    LOG_INST() << "intrinsics::ldthisbyname stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << receiver.GetRawData();

    if (LIKELY(receiver.IsHeapObject())) {
        // fast path
        JSTaggedValue res = FastRuntimeStub::GetPropertyByName(thread, receiver, propKey);
        if (!res.IsHole()) {
            ASSERT(!res.IsAccessor());
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDTHISBYNAME_IMM8_ID16);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::LdObjByName(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDTHISBYNAME_IMM8_ID16);
}

void InterpreterAssembly::HandleLdexternalmodulevarImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_8_0();
    LOG_INST() << "intrinsics::ldmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdExternalModuleVar(thread, index);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(LDEXTERNALMODULEVAR_IMM8);
}

void InterpreterAssembly::HandleCallRuntimeLdsendableexternalmodulevarImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_8_1();
    JSTaggedValue thisFunc = GetFunction(sp);
    LOG_INST() << "intrinsics::ldsendableexternalmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdSendableExternalModuleVar(thread, index, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(CALLRUNTIME_LDSENDABLEEXTERNALMODULEVAR_PREF_IMM8);
}

void InterpreterAssembly::HandleCallRuntimeNewSendableEnvImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint8_t numVars = READ_INST_8_1();
    LOG_INST() << "intrinsics::newsendableenv"
               << " imm " << numVars;
    JSTaggedValue res = SlowRuntimeStub::NewSendableEnv(thread, numVars);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    SourceTextModule *moduleRecord = SourceTextModule::Cast(GetModule(sp));
    moduleRecord->SetSendableEnv(thread, res);
    DISPATCH(CALLRUNTIME_NEWSENDABLEENV_PREF_IMM8);
}

void InterpreterAssembly::HandleCallRuntimeNewSendableEnvImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t numVars = READ_INST_16_1();
    LOG_INST() << "intrinsics::newsendableenv"
                << " imm " << numVars;

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewSendableEnv(thread, numVars);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    SourceTextModule *moduleRecord = SourceTextModule::Cast(GetModule(sp));
    moduleRecord->SetSendableEnv(thread, res);
    DISPATCH(CALLRUNTIME_WIDENEWSENDABLEENV_PREF_IMM16);
}

void InterpreterAssembly::HandleCallRuntimeStSendableVarImm4Imm4(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_4_2();
    uint16_t slot = READ_INST_4_3();
    LOG_INST() << "intrinsics::stsendablevar"
               << " level:" << level << " slot:" << slot;

    JSTaggedValue value = GET_ACC();
    SourceTextModule *moduleRecord = SourceTextModule::Cast(GetModule(sp));
    JSTaggedValue env = moduleRecord->GetSendableEnv();
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = SendableEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SendableEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(CALLRUNTIME_STSENDABLEVAR_PREF_IMM4_IMM4);
}

void InterpreterAssembly::HandleCallRuntimeStSendableVarImm8Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_8_1();
    uint16_t slot = READ_INST_8_2();
    LOG_INST() << "intrinsics::stsendablevar"
                << " level:" << level << " slot:" << slot;

    JSTaggedValue value = GET_ACC();
    SourceTextModule *moduleRecord = SourceTextModule::Cast(GetModule(sp));
    JSTaggedValue env = moduleRecord->GetSendableEnv();
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = SendableEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SendableEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(CALLRUNTIME_STSENDABLEVAR_PREF_IMM8_IMM8);
}

void InterpreterAssembly::HandleCallRuntimeStSendableVarImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_16_1();
    uint16_t slot = READ_INST_16_3();
    LOG_INST() << "intrinsics::stsendablevar"
                << " level:" << level << " slot:" << slot;

    JSTaggedValue value = GET_ACC();
    SourceTextModule *moduleRecord = SourceTextModule::Cast(GetModule(sp));
    JSTaggedValue env = moduleRecord->GetSendableEnv();
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = SendableEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SendableEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

    DISPATCH(CALLRUNTIME_WIDESTSENDABLEVAR_PREF_IMM16_IMM16);
}

void InterpreterAssembly::HandleCallRuntimeLdSendableVarImm4Imm4(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_4_2();
    uint16_t slot = READ_INST_4_3();

    LOG_INST() << "intrinsics::ldsendablevar"
               << " level:" << level << " slot:" << slot;
    SourceTextModule *moduleRecord = SourceTextModule::Cast(GetModule(sp));
    JSTaggedValue env = moduleRecord->GetSendableEnv();
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = SendableEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SET_ACC(SendableEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
    DISPATCH(CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4);
}

void InterpreterAssembly::HandleCallRuntimeLdSendableVarImm8Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_8_1();
    uint16_t slot = READ_INST_8_2();

    LOG_INST() << "intrinsics::ldsendablevar"
                << " level:" << level << " slot:" << slot;
    SourceTextModule *moduleRecord = SourceTextModule::Cast(GetModule(sp));
    JSTaggedValue env = moduleRecord->GetSendableEnv();
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = SendableEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SET_ACC(SendableEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
    DISPATCH(CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8);
}

void InterpreterAssembly::HandleCallRuntimeLdSendableVarImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_16_1();
    uint16_t slot = READ_INST_16_3();

    LOG_INST() << "intrinsics::ldsendablevar"
                << " level:" << level << " slot:" << slot;
    SourceTextModule *moduleRecord = SourceTextModule::Cast(GetModule(sp));
    JSTaggedValue env = moduleRecord->GetSendableEnv();
    for (uint32_t i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = SendableEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    SET_ACC(SendableEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
    DISPATCH(CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16);
}

void InterpreterAssembly::HandleDefinemethodImm16Id16Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_2();
    uint16_t length = READ_INST_8_4();
    LOG_INST() << "intrinsics::definemethod length: " << length;
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    Method *method =
        Method::Cast(ConstantPool::GetMethodFromCache(thread, constpool, methodId).GetTaggedObject());
    ASSERT(method != nullptr);
    RESTORE_ACC();

    SAVE_PC();
    JSTaggedValue homeObject = GET_ACC();
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue taggedCurEnv = state->env;

    auto res = SlowRuntimeStub::DefineMethod(thread, method, homeObject, length, taggedCurEnv, GetModule(sp));
    INTERPRETER_RETURN_IF_ABRUPT(res);
    JSFunction *result = JSFunction::Cast(res.GetTaggedObject());

    SET_ACC(JSTaggedValue(result));
    DISPATCH(DEFINEMETHOD_IMM16_ID16_IMM8);
}

void InterpreterAssembly::HandleDeprecatedCallrangePrefImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEPRECATED_CALLRANGE_PREF_IMM16_V8);
}

void InterpreterAssembly::HandleCallrangeImm8Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLRANGE_IMM8_IMM8_V8);
}

void InterpreterAssembly::HandleDynamicimport(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::dynamicimport";
    JSTaggedValue specifier = GET_ACC();
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::DynamicImport(thread, specifier, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DYNAMICIMPORT);
}

void InterpreterAssembly::HandleDeprecatedDynamicimportPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::dynamicimport"
                << " v" << v0;
    JSTaggedValue specifier = GET_VREG_VALUE(v0);
    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::DynamicImport(thread, specifier, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEPRECATED_DYNAMICIMPORT_PREF_V8);
}

void InterpreterAssembly::HandleCallargs3Imm8V8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLARGS3_IMM8_V8_V8_V8);
}

void InterpreterAssembly::HandleCallargs2Imm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLARGS2_IMM8_V8_V8);
}

void InterpreterAssembly::HandleApplyImm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    uint16_t v1 = READ_INST_8_1();
    LOG_INST() << "intrinsics::callspread"
                << " v" << v0 << " v" << v1;
    JSTaggedValue func = GET_ACC();
    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue array = GET_VREG_VALUE(v1);

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CallSpread(thread, func, obj, array);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);

    DISPATCH(APPLY_IMM8_V8_V8);
}

void InterpreterAssembly::HandleCallarg0Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLARG0_IMM8);
}

void InterpreterAssembly::HandleDefinemethodImm8Id16Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_8_3();
    LOG_INST() << "intrinsics::definemethod length: " << length;
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    Method *method =
        Method::Cast(ConstantPool::GetMethodFromCache(thread, constpool, methodId).GetTaggedObject());
    ASSERT(method != nullptr);
    RESTORE_ACC();

    SAVE_PC();
    JSTaggedValue homeObject = GET_ACC();
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue taggedCurEnv = state->env;
    auto res = SlowRuntimeStub::DefineMethod(thread, method, homeObject, length, taggedCurEnv, GetModule(sp));
    INTERPRETER_RETURN_IF_ABRUPT(res);
    JSFunction *result = JSFunction::Cast(res.GetTaggedObject());

    SET_ACC(JSTaggedValue(result));
    DISPATCH(DEFINEMETHOD_IMM8_ID16_IMM8);
}

void InterpreterAssembly::HandleDefinefuncImm16Id16Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_2();
    uint16_t length = READ_INST_8_4();
    LOG_INST() << "intrinsics::definefunc length: " << length;

    constpool = GetConstantPool(sp);
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    JSTaggedValue envHandle = state->env;
    JSFunction *currentFunc =
        JSFunction::Cast(((reinterpret_cast<InterpretedFrame *>(sp) - 1)->function).GetTaggedObject());

    auto res = SlowRuntimeStub::DefineFunc(thread, constpool, methodId, GetModule(sp),
                                           length, envHandle, currentFunc->GetHomeObject());
    JSFunction *jsFunc = JSFunction::Cast(res.GetTaggedObject());
    SET_ACC(JSTaggedValue(jsFunc));

    DISPATCH(DEFINEFUNC_IMM16_ID16_IMM8);
}

void InterpreterAssembly::HandleDefinefuncImm8Id16Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_8_3();
    LOG_INST() << "intrinsics::definefunc length: " << length;

    constpool = GetConstantPool(sp);
    AsmInterpretedFrame *state = (reinterpret_cast<AsmInterpretedFrame *>(sp) - 1);
    JSTaggedValue envHandle = state->env;
    JSFunction *currentFunc =
        JSFunction::Cast(((reinterpret_cast<AsmInterpretedFrame *>(sp) - 1)->function).GetTaggedObject());

    auto res = SlowRuntimeStub::DefineFunc(thread, constpool, methodId, GetModule(sp),
                                           length, envHandle, currentFunc->GetHomeObject());
    JSFunction *jsFunc = JSFunction::Cast(res.GetTaggedObject());

    SET_ACC(JSTaggedValue(jsFunc));
    DISPATCH(DEFINEFUNC_IMM8_ID16_IMM8);
}

void InterpreterAssembly::HandleSupercallarrowrangeImm8Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t range = READ_INST_8_1();
    uint16_t v0 = READ_INST_8_2();
    LOG_INST() << "intrinsics::supercall"
               << " range: " << range << " v" << v0;

    JSTaggedValue thisFunc = GET_ACC();
    JSTaggedValue newTarget = GetNewTarget(sp);

    SAVE_PC();
    JSTaggedValue superCtor = SlowRuntimeStub::GetSuperConstructor(thread, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(superCtor);
    JSMutableHandle<Method> methodHandle(thread, JSTaggedValue::Undefined());

    if (superCtor.IsJSFunction() && superCtor.IsConstructor() && !newTarget.IsUndefined()) {
        JSFunction *superCtorFunc = JSFunction::Cast(superCtor.GetTaggedObject());
        methodHandle.Update(superCtorFunc->GetMethod());
        if (superCtorFunc->IsBuiltinConstructor()) {
            ASSERT(methodHandle->GetNumVregsWithCallField() == 0);
            size_t frameSize =
                InterpretedFrame::NumOfMembers() + range + NUM_MANDATORY_JSFUNC_ARGS + 2; // 2:thread & numArgs
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            // copy args
            uint32_t index = 0;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(newSp);
            newSp[index++] = ToUintPtr(thread);
            newSp[index++] = range + NUM_MANDATORY_JSFUNC_ARGS;
            // func
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = superCtor.GetRawData();
            // newTarget
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = newTarget.GetRawData();
            // this
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            for (size_t i = 0; i < range; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(v0 + i);
            }

            InterpretedBuiltinFrame *state = GET_BUILTIN_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_BUILTIN_FRAME;
            state->pc = nullptr;
            state->function = superCtor;
            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime SuperCall ";
            JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                superCtorFunc->GetNativePointer())(ecmaRuntimeCallInfo);
            thread->SetCurrentSPFrame(sp);

            if (UNLIKELY(thread->HasPendingException())) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            LOG_INST() << "Exit: Runtime SuperCall ";
            SET_ACC(retValue);
            DISPATCH(SUPERCALLARROWRANGE_IMM8_IMM8_V8);
        }

        if (AssemblyIsFastNewFrameEnter(superCtorFunc, methodHandle)) {
            SAVE_PC();
            uint32_t numVregs = methodHandle->GetNumVregsWithCallField();
            uint32_t numDeclaredArgs = superCtorFunc->IsBase() ?
                methodHandle->GetNumArgsWithCallField() + 1 :  // +1 for this
                methodHandle->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
            // +1 for hidden this, explicit this may be overwritten after bc optimizer
            size_t frameSize = InterpretedFrame::NumOfMembers() + numVregs + numDeclaredArgs + 1;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(newSp) - 1;

            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            uint32_t index = 0;
            // initialize vregs value
            for (size_t i = 0; i < numVregs; ++i) {
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            // this
            JSTaggedValue thisObj;
            if (superCtorFunc->IsBase()) {
                thisObj = FastRuntimeStub::NewThisObject(thread, superCtor, newTarget, state);
                INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();
            } else {
                ASSERT(superCtorFunc->IsDerivedConstructor());
                newSp[index++] = newTarget.GetRawData();
                thisObj = JSTaggedValue::Undefined();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();

                state->function = superCtor;
                state->constpool = methodHandle->GetConstantPool();
                state->profileTypeInfo = superCtorFunc->GetProfileTypeInfo();
                state->env = superCtorFunc->GetLexicalEnv();
            }

            // the second condition ensure not push extra args
            for (size_t i = 0; i < range && index < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(v0 + i);
            }

            // set undefined to the extra prats of declare
            for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
            state->thisObj = thisObj;
            state->pc = pc = methodHandle->GetBytecodeArray();
            sp = newSp;
            state->acc = JSTaggedValue::Hole();

            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime SuperCall " << std::hex << reinterpret_cast<uintptr_t>(sp)
                                    << " " << std::hex << reinterpret_cast<uintptr_t>(pc);
            DISPATCH_OFFSET(0);
        }
    }

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::SuperCall(thread, thisFunc, newTarget, v0, range);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(SUPERCALLARROWRANGE_IMM8_IMM8_V8);
}

void InterpreterAssembly::HandleSupercallthisrangeImm8Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t range = READ_INST_8_1();
    uint16_t v0 = READ_INST_8_2();
    LOG_INST() << "intrinsics::supercall"
               << " range: " << range << " v" << v0;

    JSTaggedValue thisFunc = GetFunction(sp);
    JSTaggedValue newTarget = GetNewTarget(sp);

    SAVE_PC();
    JSTaggedValue superCtor = SlowRuntimeStub::GetSuperConstructor(thread, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(superCtor);

    JSMutableHandle<Method> methodHandle(thread, JSTaggedValue::Undefined());
    if (superCtor.IsJSFunction() && superCtor.IsConstructor() && !newTarget.IsUndefined()) {
        JSFunction *superCtorFunc = JSFunction::Cast(superCtor.GetTaggedObject());
        methodHandle.Update(superCtorFunc->GetMethod());
        if (superCtorFunc->IsBuiltinConstructor()) {
            ASSERT(methodHandle->GetNumVregsWithCallField() == 0);
            size_t frameSize =
                InterpretedFrame::NumOfMembers() + range + NUM_MANDATORY_JSFUNC_ARGS + 2; // 2:thread & numArgs
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            // copy args
            uint32_t index = 0;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(newSp);
            newSp[index++] = ToUintPtr(thread);
            newSp[index++] = range + NUM_MANDATORY_JSFUNC_ARGS;
            // func
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = superCtor.GetRawData();
            // newTarget
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = newTarget.GetRawData();
            // this
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            for (size_t i = 0; i < range; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(v0 + i);
            }

            InterpretedBuiltinFrame *state = GET_BUILTIN_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_BUILTIN_FRAME;
            state->pc = nullptr;
            state->function = superCtor;
            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime SuperCall ";
            JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                superCtorFunc->GetNativePointer())(ecmaRuntimeCallInfo);
            thread->SetCurrentSPFrame(sp);

            if (UNLIKELY(thread->HasPendingException())) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            LOG_INST() << "Exit: Runtime SuperCall ";
            SET_ACC(retValue);
            DISPATCH(SUPERCALLTHISRANGE_IMM8_IMM8_V8);
        }

        if (AssemblyIsFastNewFrameEnter(superCtorFunc, methodHandle)) {
            SAVE_PC();
            uint32_t numVregs = methodHandle->GetNumVregsWithCallField();
            uint32_t numDeclaredArgs = superCtorFunc->IsBase() ?
                methodHandle->GetNumArgsWithCallField() + 1 :  // +1 for this
                methodHandle->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
            // +1 for hidden this, explicit this may be overwritten after bc optimizer
            size_t frameSize = InterpretedFrame::NumOfMembers() + numVregs + numDeclaredArgs + 1;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(newSp) - 1;

            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            uint32_t index = 0;
            // initialize vregs value
            for (size_t i = 0; i < numVregs; ++i) {
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            // this
            JSTaggedValue thisObj;
            if (superCtorFunc->IsBase()) {
                thisObj = FastRuntimeStub::NewThisObject(thread, superCtor, newTarget, state);
                INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();
            } else {
                ASSERT(superCtorFunc->IsDerivedConstructor());
                newSp[index++] = newTarget.GetRawData();
                thisObj = JSTaggedValue::Undefined();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();

                state->function = superCtor;
                state->constpool = methodHandle->GetConstantPool();
                state->profileTypeInfo = superCtorFunc->GetProfileTypeInfo();
                state->env = superCtorFunc->GetLexicalEnv();
            }

            // the second condition ensure not push extra args
            for (size_t i = 0; i < range && index < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(v0 + i);
            }

            // set undefined to the extra prats of declare
            for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
            state->thisObj = thisObj;
            state->pc = pc = methodHandle->GetBytecodeArray();
            sp = newSp;
            state->acc = JSTaggedValue::Hole();

            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime SuperCall " << std::hex << reinterpret_cast<uintptr_t>(sp)
                                    << " " << std::hex << reinterpret_cast<uintptr_t>(pc);
            DISPATCH_OFFSET(0);
        }
    }

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::SuperCall(thread, thisFunc, newTarget, v0, range);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(SUPERCALLTHISRANGE_IMM8_IMM8_V8);
}

void InterpreterAssembly::HandleCallthisrangeImm8Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLTHISRANGE_IMM8_IMM8_V8);
}

void InterpreterAssembly::HandleCallthis3Imm8V8V8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLTHIS3_IMM8_V8_V8_V8_V8);
}

void InterpreterAssembly::HandleCallthis2Imm8V8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLTHIS2_IMM8_V8_V8_V8);
}

void InterpreterAssembly::HandleNewlexenvwithnameImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t numVars = READ_INST_8_0();
    uint16_t scopeId = READ_INST_16_1();
    LOG_INST() << "intrinsics::newlexenvwithname"
               << " numVars " << numVars << " scopeId " << scopeId;

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewLexicalEnvWithName(thread, numVars, scopeId);
    INTERPRETER_RETURN_IF_ABRUPT(res);

    SET_ACC(res);
    (reinterpret_cast<InterpretedFrame *>(sp) - 1)->env = res;
    DISPATCH(NEWLEXENVWITHNAME_IMM8_ID16);
}

void InterpreterAssembly::HandleNewobjrangeImm16Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t numArgs = READ_INST_8_2();
    uint16_t firstArgRegIdx = READ_INST_8_3();
    LOG_INST() << "intrinsics::newobjRange " << numArgs << " v" << firstArgRegIdx;
    JSTaggedValue ctor = GET_VREG_VALUE(firstArgRegIdx);
    JSMutableHandle<Method> methodHandle(thread, JSTaggedValue::Undefined());

    if (ctor.IsJSFunction() && ctor.IsConstructor()) {
        JSFunction *ctorFunc = JSFunction::Cast(ctor.GetTaggedObject());
        methodHandle.Update(ctorFunc->GetMethod());
        if (ctorFunc->IsBuiltinConstructor()) {
            ASSERT(methodHandle->GetNumVregsWithCallField() == 0);
            size_t frameSize =
                InterpretedFrame::NumOfMembers() + numArgs + 4; // 4: newtarget/this & numArgs & thread
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            // copy args
            uint32_t index = 0;
            // numArgs
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo*>(newSp);
            newSp[index++] = ToUintPtr(thread);
            newSp[index++] = numArgs + 2; // 2: for newtarget/this
            // func
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = ctor.GetRawData();
            // newTarget
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = ctor.GetRawData();
            // this
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            for (size_t i = 1; i < numArgs; ++i) {  // 1: func
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(firstArgRegIdx + i);
            }

            InterpretedBuiltinFrame *state = GET_BUILTIN_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_BUILTIN_FRAME;
            state->pc = nullptr;
            state->function = ctor;
            thread->SetCurrentSPFrame(newSp);

            LOG_INST() << "Entry: Runtime New.";
            SAVE_PC();
            JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                ctorFunc->GetNativePointer())(ecmaRuntimeCallInfo);
            thread->SetCurrentSPFrame(sp);
            if (UNLIKELY(thread->HasPendingException())) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            LOG_INST() << "Exit: Runtime New.";
            SET_ACC(retValue);
            DISPATCH(NEWOBJRANGE_IMM16_IMM8_V8);
        }

        if (AssemblyIsFastNewFrameEnter(ctorFunc, methodHandle)) {
            SAVE_PC();
            uint32_t numVregs = methodHandle->GetNumVregsWithCallField();
            uint32_t numDeclaredArgs = ctorFunc->IsBase() ?
                                       methodHandle->GetNumArgsWithCallField() + 1 :  // +1 for this
                                       methodHandle->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
            size_t frameSize = InterpretedFrame::NumOfMembers() + numVregs + numDeclaredArgs;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(newSp) - 1);

            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            uint32_t index = 0;
            // initialize vregs value
            for (size_t i = 0; i < numVregs; ++i) {
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            // this
            JSTaggedValue thisObj;
            if (ctorFunc->IsBase()) {
                thisObj = FastRuntimeStub::NewThisObject(thread, ctor, ctor, state);
                INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();
            } else {
                ASSERT(ctorFunc->IsDerivedConstructor());
                newSp[index++] = ctor.GetRawData();
                thisObj = JSTaggedValue::Undefined();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();

                state->function = ctor;
                state->constpool = methodHandle->GetConstantPool();
                state->profileTypeInfo = ctorFunc->GetProfileTypeInfo();
                state->env = ctorFunc->GetLexicalEnv();
            }

            // the second condition ensure not push extra args
            for (size_t i = 1; i < numArgs && index < numVregs + numDeclaredArgs; ++i) {  // 2: func and newTarget
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(firstArgRegIdx + i);
            }

            // set undefined to the extra prats of declare
            for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
            state->thisObj = thisObj;
            state->pc = pc = methodHandle->GetBytecodeArray();
            sp = newSp;
            state->acc = JSTaggedValue::Hole();

            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime New " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                                    << std::hex << reinterpret_cast<uintptr_t>(pc);
            DISPATCH_OFFSET(0);
        }
    }

    // bound function, proxy, other call types, enter slow path
    constexpr uint16_t firstArgOffset = 1;
    // Exclude func and newTarget
    uint16_t firstArgIdx = firstArgRegIdx + firstArgOffset;
    uint16_t length = numArgs - firstArgOffset;

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewObjRange(thread, ctor, ctor, firstArgIdx, length);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(NEWOBJRANGE_IMM16_IMM8_V8);
}

void InterpreterAssembly::HandleNewobjrangeImm8Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t numArgs = READ_INST_8_1();
    uint16_t firstArgRegIdx = READ_INST_8_2();
    LOG_INST() << "intrinsics::newobjRange " << numArgs << " v" << firstArgRegIdx;
    JSTaggedValue ctor = GET_VREG_VALUE(firstArgRegIdx);
    JSMutableHandle<Method> methodHandle(thread, JSTaggedValue::Undefined());

    if (ctor.IsJSFunction() && ctor.IsConstructor()) {
        JSFunction *ctorFunc = JSFunction::Cast(ctor.GetTaggedObject());
        methodHandle.Update(ctorFunc->GetMethod());
        if (ctorFunc->IsBuiltinConstructor()) {
            ASSERT(methodHandle->GetNumVregsWithCallField() == 0);
            size_t frameSize = InterpretedFrame::NumOfMembers() + numArgs + 4;  // 2: numArgs & thread
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            // copy args
            uint32_t index = 0;
            // numArgs
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo*>(newSp);
            newSp[index++] = ToUintPtr(thread);
            newSp[index++] = numArgs + 2; // 2: for newtarget / this
            // func
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = ctor.GetRawData();
            // newTarget
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = ctor.GetRawData();
            // this
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            for (size_t i = 1; i < numArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(firstArgRegIdx + i);
            }

            InterpretedBuiltinFrame *state = GET_BUILTIN_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_BUILTIN_FRAME;
            state->pc = nullptr;
            state->function = ctor;
            thread->SetCurrentSPFrame(newSp);

            LOG_INST() << "Entry: Runtime New.";
            SAVE_PC();
            JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                ctorFunc->GetNativePointer())(ecmaRuntimeCallInfo);
            thread->SetCurrentSPFrame(sp);
            if (UNLIKELY(thread->HasPendingException())) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            LOG_INST() << "Exit: Runtime New.";
            SET_ACC(retValue);
            DISPATCH(NEWOBJRANGE_IMM8_IMM8_V8);
        }

        if (AssemblyIsFastNewFrameEnter(ctorFunc, methodHandle)) {
            SAVE_PC();
            uint32_t numVregs = methodHandle->GetNumVregsWithCallField();
            uint32_t numDeclaredArgs = ctorFunc->IsBase() ?
                                       methodHandle->GetNumArgsWithCallField() + 1 :  // +1 for this
                                       methodHandle->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
            size_t frameSize = InterpretedFrame::NumOfMembers() + numVregs + numDeclaredArgs;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            JSTaggedType *newSp = sp - frameSize;
            InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(newSp) - 1);

            if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            uint32_t index = 0;
            // initialize vregs value
            for (size_t i = 0; i < numVregs; ++i) {
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            // this
            JSTaggedValue thisObj;
            if (ctorFunc->IsBase()) {
                thisObj = FastRuntimeStub::NewThisObject(thread, ctor, ctor, state);
                INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();
            } else {
                ASSERT(ctorFunc->IsDerivedConstructor());
                newSp[index++] = ctor.GetRawData();
                thisObj = JSTaggedValue::Undefined();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = thisObj.GetRawData();

                state->function = ctor;
                state->constpool = methodHandle->GetConstantPool();
                state->profileTypeInfo = ctorFunc->GetProfileTypeInfo();
                state->env = ctorFunc->GetLexicalEnv();
            }

            // the second condition ensure not push extra args
            for (size_t i = 1; i < numArgs && index < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(firstArgRegIdx + i);
            }

            // set undefined to the extra prats of declare
            for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
            }

            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
            state->thisObj = thisObj;
            state->pc = pc = methodHandle->GetBytecodeArray();
            sp = newSp;
            state->acc = JSTaggedValue::Hole();

            thread->SetCurrentSPFrame(newSp);
            LOG_INST() << "Entry: Runtime New " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                                    << std::hex << reinterpret_cast<uintptr_t>(pc);
            DISPATCH_OFFSET(0);
        }
    }

    // bound function, proxy, other call types, enter slow path
    constexpr uint16_t firstArgOffset = 1;
    // Exclude func and newTarget
    uint16_t firstArgIdx = firstArgRegIdx + firstArgOffset;
    uint16_t length = numArgs - firstArgOffset;

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewObjRange(thread, ctor, ctor, firstArgIdx, length);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(NEWOBJRANGE_IMM8_IMM8_V8);
}

void InterpreterAssembly::HandleNewobjapplyImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_2();
    LOG_INST() << "intrinsic::newobjspeard"
               << " v" << v0;
    JSTaggedValue func = GET_VREG_VALUE(v0);
    JSTaggedValue array = GET_ACC();
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewObjApply(thread, func, array);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(NEWOBJAPPLY_IMM16_V8);
}

void InterpreterAssembly::HandleCreateregexpwithliteralImm16Id16Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_2();
    SAVE_ACC();
    constpool = GetConstantPool(sp);
    JSTaggedValue pattern = ConstantPool::GetStringFromCache(thread, constpool, stringId);
    uint8_t flags = READ_INST_8_4();
    LOG_INST() << "intrinsics::createregexpwithliteral "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(pattern.GetTaggedObject()))
               << ", flags:" << flags;
    JSTaggedValue res = SlowRuntimeStub::CreateRegExpWithLiteral(thread, pattern, flags);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8);
}

void InterpreterAssembly::HandleCreateobjectwithbufferImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_2();
    LOG_INST() << "intrinsics::createobjectwithbuffer"
               << " imm:" << imm;
    constpool = GetUnsharedConstpool(thread, sp);
    JSObject *result = JSObject::Cast(
        ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(
            thread, constpool, imm, GetModule(sp)).GetTaggedObject());
    SAVE_PC();
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateObjectHavingMethod(thread, factory, result, state->env);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEOBJECTWITHBUFFER_IMM16_ID16);
}

void InterpreterAssembly::HandleCreateobjectwithbufferImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    LOG_INST() << "intrinsics::createobjectwithbuffer"
               << " imm:" << imm;
    constpool = GetUnsharedConstpool(thread, sp);
    JSObject *result = JSObject::Cast(
        ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(
            thread, constpool, imm, GetModule(sp)).GetTaggedObject());
    SAVE_PC();
    InterpretedFrame *state = (reinterpret_cast<InterpretedFrame *>(sp) - 1);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateObjectHavingMethod(thread, factory, result, state->env);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEOBJECTWITHBUFFER_IMM8_ID16);
}

void InterpreterAssembly::HandleLdnewtarget(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(LDNEWTARGET);
}

void InterpreterAssembly::HandleLdthis(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldthis";
    SET_ACC(GetThis(sp));
    DISPATCH(LDTHIS);
}

void InterpreterAssembly::HandleCreatearraywithbufferImm8Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    LOG_INST() << "intrinsics::createarraywithbuffer"
               << " imm:" << imm;
    constpool = GetUnsharedConstpool(thread, sp);
    JSArray *result = JSArray::Cast(
        ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(
            thread, constpool, imm, GetModule(sp)).GetTaggedObject());
    SAVE_PC();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateArrayWithBuffer(thread, factory, result);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEARRAYWITHBUFFER_IMM8_ID16);
}

void InterpreterAssembly::HandleCreatearraywithbufferImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_2();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    LOG_INST() << "intrinsics::createarraywithbuffer"
               << " imm:" << imm;
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    JSTaggedValue constantPool = state->constpool;
    JSArray *result = JSArray::Cast(ConstantPool::Cast(constantPool.GetTaggedObject())
        ->GetObjectFromCache(imm).GetTaggedObject());
    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CreateArrayWithBuffer(thread, factory, result);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEARRAYWITHBUFFER_IMM16_ID16);
}

void InterpreterAssembly::HandleCallthis0Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLTHIS0_IMM8_V8);
}

void InterpreterAssembly::HandleCallthis1Imm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CALLTHIS1_IMM8_V8_V8);
}

void InterpreterAssembly::HandleNop(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::nop";
    DISPATCH(NOP);
}

void InterpreterAssembly::ExceptionHandler(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    FrameHandler frameHandler(thread);
    uint32_t pcOffset = panda_file::INVALID_OFFSET;
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            thread->SetLastFp(frameHandler.GetFp());
            return;
        }
        auto method = frameHandler.GetMethod();
        pcOffset = method->FindCatchBlock(frameHandler.GetBytecodeOffset());
        if (pcOffset != INVALID_INDEX) {
            thread->SetCurrentFrame(frameHandler.GetSp());
            thread->SetLastFp(frameHandler.GetFp());
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            pc = method->GetBytecodeArray() + pcOffset;
            break;
        }
    }
    if (pcOffset == INVALID_INDEX) {
        return;
    }

    auto exception = thread->GetException();
    SET_ACC(exception);
    thread->ClearException();
    DISPATCH_OFFSET(0);
}

void InterpreterAssembly::HandleCallRuntimeLdLazyModuleVarPrefImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_8_1();
    JSTaggedValue thisFunc = GetFunction(sp);
    LOG_INST() << "intrinsics::ldlazyexternalmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdLazyExternalModuleVar(thread, index, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8);
}

void InterpreterAssembly::HandleCallRuntimeWideLdLazyModuleVarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_16_1();
    JSTaggedValue thisFunc = GetFunction(sp);
    LOG_INST() << "intrinsics::ldlazyexternalmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdLazyExternalModuleVar(thread, index, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16);
}

void InterpreterAssembly::HandleCallRuntimeLdLazySendableModuleVarPrefImm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_8_1();
    JSTaggedValue thisFunc = GetFunction(sp);
    LOG_INST() << "intrinsics::ldlazysendableexternalmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdLazySendableExternalModuleVar(thread, index, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(CALLRUNTIME_LDLAZYSENDABLEMODULEVAR_PREF_IMM8);
}

void InterpreterAssembly::HandleCallRuntimeWideLdLazySendableModuleVarPrefImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    int32_t index = READ_INST_16_1();
    JSTaggedValue thisFunc = GetFunction(sp);
    LOG_INST() << "intrinsics::ldlazysendableexternalmodulevar index:" << index;

    JSTaggedValue moduleVar = SlowRuntimeStub::LdLazySendableExternalModuleVar(thread, index, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(CALLRUNTIME_WIDELDLAZYSENDABLEMODULEVAR_PREF_IMM16);
}

#define DECLARE_UNUSED_ASM_HANDLE(name)                                                 \
    void InterpreterAssembly::name(                                                     \
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, \
        JSTaggedValue profileTypeInfo, JSTaggedValue acc, int16_t hotnessCounter)       \
    {                                                                                   \
        LOG_INTERPRETER(FATAL) << #name;                                                \
    }
ASM_UNUSED_BC_STUB_LIST(DECLARE_UNUSED_ASM_HANDLE)
#undef DECLARE_UNUSED_ASM_HANDLE
#endif

inline void InterpreterAssembly::InterpreterFrameCopyArgs(
    JSTaggedType *newSp, uint32_t numVregs, uint32_t numActualArgs, uint32_t numDeclaredArgs, bool haveExtraArgs)
{
    size_t i = 0;
    for (; i < numVregs; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        newSp[i] = JSTaggedValue::VALUE_UNDEFINED;
    }
    for (i = numActualArgs; i < numDeclaredArgs; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        newSp[numVregs + i] = JSTaggedValue::VALUE_UNDEFINED;
    }
    if (haveExtraArgs) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        newSp[numVregs + i] = JSTaggedValue(numActualArgs).GetRawData();  // numActualArgs is stored at the end
    }
}

JSTaggedValue InterpreterAssembly::GetFunction(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    return JSTaggedValue(state->function);
}

JSTaggedValue InterpreterAssembly::GetThis(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    return JSTaggedValue(state->thisObj);
}

JSTaggedValue InterpreterAssembly::GetNewTarget(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget();
    ASSERT(method->HaveNewTargetWithCallField());
    uint32_t numVregs = method->GetNumVregsWithCallField();
    bool haveFunc = method->HaveFuncWithCallField();
    return JSTaggedValue(sp[numVregs + haveFunc]);
}

JSTaggedValue InterpreterAssembly::GetConstantPool(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget();
    return method->GetConstantPool();
}

JSTaggedValue InterpreterAssembly::GetUnsharedConstpool(JSThread* thread, JSTaggedType *sp)
{
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget();
    return thread->GetEcmaVM()->FindOrCreateUnsharedConstpool(method->GetConstantPool());
}

JSTaggedValue InterpreterAssembly::GetModule(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    return JSFunction::Cast(state->function.GetTaggedObject())->GetModule();
}

JSTaggedValue InterpreterAssembly::GetProfileTypeInfo(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    JSFunction *function = JSFunction::Cast(state->function.GetTaggedObject());
    return function->GetProfileTypeInfo();
}

JSTaggedType *InterpreterAssembly::GetAsmInterpreterFramePointer(AsmInterpretedFrame *state)
{
    return state->GetCurrentFramePointer();
}

uint32_t InterpreterAssembly::GetNumArgs(JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget();
    ASSERT(method->HaveExtraWithCallField());

    uint32_t numVregs = method->GetNumVregsWithCallField();
    bool haveFunc = method->HaveFuncWithCallField();
    bool haveNewTarget = method->HaveNewTargetWithCallField();
    bool haveThis = method->HaveThisWithCallField();
    uint32_t copyArgs = haveFunc + haveNewTarget + haveThis;
    uint32_t numArgs = method->GetNumArgsWithCallField();

    JSTaggedType *fp = GetAsmInterpreterFramePointer(state);
    if (static_cast<uint32_t>(fp - sp) > numVregs + copyArgs + numArgs) {
        // In this case, actualNumArgs is in the end
        // If not, then actualNumArgs == declaredNumArgs, therefore do nothing
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        numArgs = static_cast<uint32_t>(JSTaggedValue(*(fp - 1)).GetInt());
    }
    startIdx = numVregs + copyArgs + restIdx;
    return ((numArgs > restIdx) ? (numArgs - restIdx) : 0);
}

inline size_t InterpreterAssembly::GetJumpSizeAfterCall(const uint8_t *prevPc)
{
    auto op = BytecodeInstruction(prevPc).GetOpcode();
    size_t jumpSize = BytecodeInstruction::Size(op);
    return jumpSize;
}

inline JSTaggedValue InterpreterAssembly::UpdateHotnessCounter(JSThread* thread, JSTaggedType *sp)
{
    AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
    thread->CheckSafepoint();
    JSFunction* function = JSFunction::Cast(state->function.GetTaggedObject());
    JSTaggedValue profileTypeInfo = function->GetProfileTypeInfo();
    if (profileTypeInfo.IsUndefined()) {
        return SlowRuntimeStub::NotifyInlineCache(thread, function);
    }
    return profileTypeInfo;
}
#undef LOG_INST
#undef ADVANCE_PC
#undef GOTO_NEXT
#undef DISPATCH_OFFSET
#undef GET_ASM_FRAME
#undef GET_ENTRY_FRAME
#undef SAVE_PC
#undef SAVE_ACC
#undef RESTORE_ACC
#undef INTERPRETER_GOTO_EXCEPTION_HANDLER
#undef INTERPRETER_HANDLE_RETURN
#undef UPDATE_HOTNESS_COUNTER
#undef GET_VREG
#undef GET_VREG_VALUE
#undef SET_VREG
#undef GET_ACC
#undef SET_ACC
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace panda::ecmascript
