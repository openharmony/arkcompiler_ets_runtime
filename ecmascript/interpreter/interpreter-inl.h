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

#ifndef ECMASCRIPT_INTERPRETER_INTERPRETER_INL_H
#define ECMASCRIPT_INTERPRETER_INTERPRETER_INL_H

#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_runtime_stub-inl.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/sendable_env.h"
#include "ecmascript/template_string.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif

namespace panda::ecmascript {
using CommonStubCSigns = kungfu::CommonStubCSigns;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-ptr-dereference"
#pragma clang diagnostic ignored "-Wgnu-label-as-value"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#if ECMASCRIPT_ENABLE_INTERPRETER_LOG
#define HANDLE_OPCODE(opcode)                                           \
    HANDLE_##opcode:                                                    \
    {                                                                   \
        RuntimeStubs::DebugPrintInstruction(thread->GetGlueAddr(), pc); \
    }
#else
#define HANDLE_OPCODE(opcode)       \
    HANDLE_##opcode:
#endif

#define NOPRINT_HANDLE_OPCODE(opcode) \
    HANDLE_##opcode:

#define LOG_INST() false && LOG_INTERPRETER(DEBUG)

#define DEBUG_HANDLE_OPCODE(opcode) \
    DEBUG_HANDLE_##opcode:

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ADVANCE_PC(offset) \
    pc += (offset);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic, cppcoreguidelines-macro-usage)

#define GOTO_NEXT()  // NOLINT(clang-diagnostic-gnu-label-as-value, cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISPATCH(curOpcode)                                       \
    do {                                                          \
        ADVANCE_PC(BytecodeInstruction::Size(EcmaOpcode::curOpcode))  \
        opcode = READ_INST_OP(); goto *dispatchTable[opcode];     \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISPATCH_OFFSET(offset)                               \
    do {                                                      \
        ADVANCE_PC(offset)                                    \
        opcode = READ_INST_OP(); goto *dispatchTable[opcode]; \
    } while (false)

#define DISPATCH_THROW()                  \
    do {                                  \
        opcode = *(pc + 1);               \
        goto *throwDispatchTable[opcode]; \
    } while (false)

#define DISPATCH_WIDE()                  \
    do {                                 \
        opcode = *(pc + 1);              \
        goto *wideDispatchTable[opcode]; \
    } while (false)

#define DISPATCH_DEPRECATED()                  \
    do {                                       \
        opcode = *(pc + 1);                    \
        goto *deprecatedDispatchTable[opcode]; \
    } while (false)

#define DISPATCH_CALLRUNTIME()                  \
    do {                                        \
        opcode = *(pc + 1);                     \
        goto *callRuntimeDispatchTable[opcode]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_FRAME(CurrentSp) \
    (reinterpret_cast<InterpretedFrame *>(CurrentSp) - 1)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_ENTRY_FRAME(sp) \
    (reinterpret_cast<InterpretedEntryFrame *>(sp) - 1)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#define GET_BUILTIN_FRAME(sp) \
    (reinterpret_cast<InterpretedBuiltinFrame *>(sp) - 1)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SAVE_PC() (GET_FRAME(sp)->pc = pc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SAVE_ACC() (GET_FRAME(sp)->acc = acc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RESTORE_ACC() (acc = GET_FRAME(sp)->acc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_VREG(idx) (sp[idx])  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_VREG_VALUE(idx) (JSTaggedValue(sp[idx]))  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_VREG(idx, val) (sp[idx] = (val));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#define GET_ACC() (acc)                        // NOLINT(cppcoreguidelines-macro-usage)
#define SET_ACC(val) (acc = val)               // NOLINT(cppcoreguidelines-macro-usage)

#define GET_METHOD_FROM_CACHE(index) \
    ConstantPool::GetMethodFromCache(thread, constpool, index)

#define GET_STR_FROM_CACHE(index) \
    ConstantPool::GetStringFromCache(thread, constpool, index)

#define GET_LITERA_FROM_CACHE(index, type, module) \
    ConstantPool::GetLiteralFromCache<type>(thread, constpool, index, module)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_GOTO_EXCEPTION_HANDLER()          \
    do {                                              \
        SAVE_PC();                                    \
        goto *dispatchTable[EXCEPTION_OPCODE]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_HANDLE_RETURN()                                                     \
    do {                                                                                \
        size_t jumpSize = GetJumpSizeAfterCall(pc);                                     \
        DISPATCH_OFFSET(jumpSize);                                                      \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_SWITCH_TO_DEBUGGER_TABLE()                 \
    if (ecmaVm->GetJsDebuggerManager()->IsDebugMode()) { \
        dispatchTable = debugDispatchTable.data();       \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REAL_GOTO_DISPATCH_OPCODE(opcode)                       \
    do {                                                        \
        ASSERT(static_cast<uint16_t>(opcode) <= 0xff);          \
        goto *instDispatchTable[static_cast<uint8_t>(opcode)];  \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REAL_GOTO_EXCEPTION_HANDLER()                     \
    do {                                                  \
        SAVE_PC();                                        \
        goto *instDispatchTable[EXCEPTION_OPCODE]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_RETURN_IF_ABRUPT(result)      \
    do {                                          \
        if (result.IsException()) {               \
            INTERPRETER_GOTO_EXCEPTION_HANDLER(); \
        }                                         \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define HANDLE_EXCEPTION_IF_ABRUPT_COMPLETION(_thread)    \
    do {                                                  \
        if (UNLIKELY((_thread)->HasPendingException())) { \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();         \
        }                                                 \
    } while (false)

#define JUMP_IF_ENTRYFRAME_PENDING()                \
    do {                                            \
        if (thread->IsEntryFrameDroppedPending()) { \
            thread->ResetEntryFrameDroppedState();  \
            DROPFRAME_JUMP();                       \
        }                                           \
    } while (false)

#define DROPFRAME_JUMP()                                              \
    do {                                                              \
        thread->ResetFrameDroppedState();                             \
        sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame()); \
        InterpretedFrame *state = GET_FRAME(sp);                      \
        pc = state->pc;                                               \
        RESTORE_ACC();                                                \
        DISPATCH_OFFSET(0);                                           \
    } while (false)

#define RESET_AND_JUMP_IF_DROPFRAME()                \
    do {                                             \
        if (thread->IsFrameDropped()) {              \
            if (thread->IsEntryFrameDroppedTrue()) { \
                return;                              \
            }                                        \
            DROPFRAME_JUMP();                        \
        }                                            \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NOTIFY_DEBUGGER_EVENT()          \
    do {                                 \
        JUMP_IF_ENTRYFRAME_PENDING();    \
        SAVE_ACC();                      \
        SAVE_PC();                       \
        NotifyBytecodePcChanged(thread); \
        RESET_AND_JUMP_IF_DROPFRAME();   \
        RESTORE_ACC();                   \
    } while (false)

/*
 * reasons of set acc with hole:
 * 1. acc will become illegal when new error
 * 2. debugger logic will save acc, so illegal acc will set to frame
 * 3. when debugger trigger gc, will mark an invalid acc and crash
 * 4. acc will set to exception later, so it can set to hole template
 */
#define NOTIFY_DEBUGGER_EXCEPTION_EVENT() \
    do {                                  \
        SET_ACC(JSTaggedValue::Hole());   \
        NOTIFY_DEBUGGER_EVENT();          \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_INITIALIZE()                                            \
    do {                                                                        \
        SAVE_PC();                                                              \
        thread->CheckSafepoint();                                               \
        funcTagged = sp[startReg];                                              \
        JSTaggedValue funcValue(funcTagged);                                    \
        if (!funcValue.IsCallable()) {                                          \
            {                                                                   \
                [[maybe_unused]] EcmaHandleScope handleScope(thread);           \
                JSHandle<JSTaggedValue>func(thread, funcValue);                 \
                std::string message = EcmaStringAccessor(                       \
                    JSTaggedValue::ToString(thread, func)).ToStdString();       \
                message.append(" is not callable");                             \
                JSHandle<JSObject> error = factory->GetJSError(                 \
                    ErrorType::TYPE_ERROR, message.c_str(), StackCheck::NO);    \
                thread->SetException(error.GetTaggedValue());                   \
            }                                                                   \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                               \
        }                                                                       \
        funcObject = ECMAObject::Cast(funcValue.GetTaggedObject());             \
        methodHandle.Update(JSTaggedValue(funcObject->GetCallTarget()));        \
        newSp = sp - InterpretedFrame::NumOfMembers();                          \
    } while (false)

#define CALL_INITIALIZE()                                                       \
    do {                                                                        \
        SAVE_PC();                                                              \
        SAVE_ACC();                                                             \
        thread->CheckSafepoint();                                               \
        RESTORE_ACC();                                                          \
        funcTagged = acc.GetRawData();                                          \
        JSTaggedValue funcValue = acc;                                          \
        if (!funcValue.IsCallable()) {                                          \
            {                                                                   \
                [[maybe_unused]] EcmaHandleScope handleScope(thread);           \
                JSHandle<JSTaggedValue>func(thread, funcValue);                 \
                std::string message = EcmaStringAccessor(                       \
                    JSTaggedValue::ToString(thread, func)).ToStdString();       \
                message.append(" is not callable");                             \
                JSHandle<JSObject> error = factory->GetJSError(                 \
                    ErrorType::TYPE_ERROR, message.c_str(), StackCheck::NO);    \
                thread->SetException(error.GetTaggedValue());                   \
            }                                                                   \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                               \
        }                                                                       \
        funcObject = ECMAObject::Cast(funcValue.GetTaggedObject());             \
        methodHandle.Update(JSTaggedValue(funcObject->GetCallTarget()));        \
        newSp = sp - InterpretedFrame::NumOfMembers();                          \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_UNDEFINED(n)                           \
    do {                                                 \
        for (int i = 0; i < (n); i++) {                  \
            *(--newSp) = JSTaggedValue::VALUE_UNDEFINED; \
        }                                                \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_0()          \
    do {                            \
        /* do nothing when 0 arg */ \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_0() CALL_PUSH_ARGS_0()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_1()   \
    do {                     \
        *(--newSp) = sp[a0]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_1() CALL_PUSH_ARGS_1()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_2()   \
    do {                     \
        *(--newSp) = sp[a1]; \
        CALL_PUSH_ARGS_1();  \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_2() CALL_PUSH_ARGS_2()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_3()   \
    do {                     \
        *(--newSp) = sp[a2]; \
        CALL_PUSH_ARGS_2();  \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_3() CALL_PUSH_ARGS_3()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_RANGE()                                               \
    do {                                                                     \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - actualNumArgs))) { \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        for (int i = actualNumArgs - 1; i >= 0; i--) {                           \
            *(--newSp) = sp[startReg + static_cast<uint32_t>(i)];            \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_RANGE()                                               \
    do {                                                                     \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - actualNumArgs))) { \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        for (int i = actualNumArgs; i > 0; i--) {                            \
            *(--newSp) = sp[startReg + static_cast<uint32_t>(i)];             \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_THISRANGE()                                          \
    do {                                                                     \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - actualNumArgs))) { \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        /* 1: skip this */                                                   \
        for (int i = actualNumArgs; i > 0; i--) {                        \
            *(--newSp) = sp[startReg + static_cast<uint32_t>(i)];             \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_THISRANGE()                                          \
    do {                                                                     \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - actualNumArgs))) { \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        /* 1: skip this */                                                   \
        for (int i = actualNumArgs + 1; i > 1; i--) {                        \
            *(--newSp) = sp[startReg + static_cast<uint32_t>(i)];             \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_0_NO_EXTRA() \
    do {                            \
        /* do nothing when 0 arg */ \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_0_NO_EXTRA() CALL_PUSH_ARGS_0_NO_EXTRA()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_1_NO_EXTRA()                             \
    do {                                                        \
        if (declaredNumArgs >= ActualNumArgsOfCall::CALLARG1) { \
            *(--newSp) = sp[a0];                                \
        }                                                       \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_1_NO_EXTRA() CALL_PUSH_ARGS_1_NO_EXTRA()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_2_NO_EXTRA()                              \
    do {                                                         \
        if (declaredNumArgs >= ActualNumArgsOfCall::CALLARGS2) { \
            *(--newSp) = sp[a1];                                 \
        }                                                        \
        DEPRECATED_CALL_PUSH_ARGS_1_NO_EXTRA();                             \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_2_NO_EXTRA() CALL_PUSH_ARGS_2_NO_EXTRA()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_3_NO_EXTRA()                              \
    do {                                                         \
        if (declaredNumArgs >= ActualNumArgsOfCall::CALLARGS3) { \
            *(--newSp) = sp[a2];                                 \
        }                                                        \
        DEPRECATED_CALL_PUSH_ARGS_2_NO_EXTRA();                             \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_3_NO_EXTRA() CALL_PUSH_ARGS_3_NO_EXTRA()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_RANGE_NO_EXTRA()                                      \
    do {                                                                     \
        int num = std::min(actualNumArgs, declaredNumArgs);                  \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - num))) {           \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        for (int i = num - 1; i >= 0; i--) {                                 \
            *(--newSp) = sp[startReg + static_cast<uint32_t>(i)];            \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_RANGE_NO_EXTRA()                           \
    do {                                                                     \
        int num = std::min(actualNumArgs, declaredNumArgs);                  \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - num))) {           \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        for (int i = num; i > 0; i--) {                                      \
            *(--newSp) = sp[startReg + static_cast<uint32_t>(i)];            \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_THISRANGE_NO_EXTRA()                                      \
    do {                                                                         \
        int num = std::min(actualNumArgs, declaredNumArgs);                      \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - num))) {               \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                                \
        }                                                                        \
        /* 1: skip this */                                                       \
        for (int i = num; i > 0; i--) {                                          \
            *(--newSp) = sp[startReg + static_cast<uint32_t>(i)];                \
        }                                                                        \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS_THISRANGE_NO_EXTRA()                                      \
    do {                                                                         \
        int num = std::min(actualNumArgs, declaredNumArgs);                      \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - num))) {               \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                                \
        }                                                                        \
        /* 1: skip this */                                                       \
        for (int i = num + 1; i > 1; i--) {                                      \
            *(--newSp) = sp[startReg + static_cast<uint32_t>(i)];                 \
        }                                                                        \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS(ARG_TYPE)                                                   \
    do {                                                                           \
        if (methodHandle->IsNativeWithCallField()) {                               \
            /* native, just push all args directly */                              \
            CALL_PUSH_ARGS_##ARG_TYPE();                                           \
            goto setVregsAndFrameNative;                                           \
        }                                                                          \
        int32_t declaredNumArgs =                                                  \
            static_cast<int32_t>(methodHandle->GetNumArgsWithCallField());         \
        if (actualNumArgs == declaredNumArgs) {                                    \
            /* fast path, just push all args directly */                           \
            CALL_PUSH_ARGS_##ARG_TYPE();                                           \
            goto setVregsAndFrameNotNative;                                        \
        }                                                                          \
        /* slow path */                                                            \
        if (!methodHandle->HaveExtraWithCallField()) {                             \
            /* push length = declaredNumArgs, may push undefined */                \
            CALL_PUSH_UNDEFINED(declaredNumArgs - actualNumArgs);                  \
            CALL_PUSH_ARGS_##ARG_TYPE##_NO_EXTRA();                                \
        } else {                                                                   \
            /* push actualNumArgs in the end, then all args, may push undefined */ \
            *(--newSp) = JSTaggedValue(actualNumArgs).GetRawData();                \
            CALL_PUSH_UNDEFINED(declaredNumArgs - actualNumArgs);                  \
            CALL_PUSH_ARGS_##ARG_TYPE();                                           \
        }                                                                          \
        goto setVregsAndFrameNotNative;                                            \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEPRECATED_CALL_PUSH_ARGS(ARG_TYPE)                                                   \
    do {                                                                           \
        if (methodHandle->IsNativeWithCallField()) {                                     \
            /* native, just push all args directly */                              \
            DEPRECATED_CALL_PUSH_ARGS_##ARG_TYPE();                                           \
            goto deprecatedSetVregsAndFrameNative;                                           \
        }                                                                          \
        int32_t declaredNumArgs =                                                  \
            static_cast<int32_t>(methodHandle->GetNumArgsWithCallField());               \
        if (actualNumArgs == declaredNumArgs) {                                    \
            /* fast path, just push all args directly */                           \
            DEPRECATED_CALL_PUSH_ARGS_##ARG_TYPE();                                           \
            goto deprecatedSetVregsAndFrameNotNative;                                        \
        }                                                                          \
        /* slow path */                                                            \
        if (!methodHandle->HaveExtraWithCallField()) {                                   \
            /* push length = declaredNumArgs, may push undefined */                \
            CALL_PUSH_UNDEFINED(declaredNumArgs - actualNumArgs);                  \
            DEPRECATED_CALL_PUSH_ARGS_##ARG_TYPE##_NO_EXTRA();                                \
        } else {                                                                   \
            /* push actualNumArgs in the end, then all args, may push undefined */ \
            *(--newSp) = JSTaggedValue(actualNumArgs).GetRawData();                \
            CALL_PUSH_UNDEFINED(declaredNumArgs - actualNumArgs);                  \
            DEPRECATED_CALL_PUSH_ARGS_##ARG_TYPE();                                           \
        }                                                                          \
        goto deprecatedSetVregsAndFrameNotNative;                                            \
    } while (false)

#if ECMASCRIPT_ENABLE_IC
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UPDATE_HOTNESS_COUNTER_NON_ACC(offset)   (UpdateHotnessCounter(thread, sp, acc, offset))

#define UPDATE_HOTNESS_COUNTER(offset)                       \
    do {                                                     \
        if (UpdateHotnessCounter(thread, sp, acc, offset)) { \
            HANDLE_EXCEPTION_IF_ABRUPT_COMPLETION(thread);   \
            RESTORE_ACC();                                   \
        }                                                    \
    } while (false)
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UPDATE_HOTNESS_COUNTER(offset) static_cast<void>(0)
#define UPDATE_HOTNESS_COUNTER_NON_ACC(offset) static_cast<void>(0)
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
    currentInst <<= 8;                            \
    currentInst += READ_INST_8(offset);           \

#define READ_INST_16_0() READ_INST_16(2)
#define READ_INST_16_1() READ_INST_16(3)
#define READ_INST_16_2() READ_INST_16(4)
#define READ_INST_16_3() READ_INST_16(5)
#define READ_INST_16_4() READ_INST_16(6)
#define READ_INST_16_5() READ_INST_16(7)
#define READ_INST_16_6() READ_INST_16(8)
#define READ_INST_16_7() READ_INST_16(9)
#define READ_INST_16(offset)                          \
    ({                                                \
        uint16_t currentInst = READ_INST_8(offset);   \
        MOVE_AND_READ_INST_8(currentInst, offset - 1) \
    })

#define READ_INST_32_0() READ_INST_32(4)
#define READ_INST_32_1() READ_INST_32(5)
#define READ_INST_32_2() READ_INST_32(6)
#define READ_INST_32(offset)                          \
    ({                                                \
        uint32_t currentInst = READ_INST_8(offset);   \
        MOVE_AND_READ_INST_8(currentInst, offset - 1) \
        MOVE_AND_READ_INST_8(currentInst, offset - 2) \
        MOVE_AND_READ_INST_8(currentInst, offset - 3) \
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

#undef LOG_INST
#undef HANDLE_OPCODE
#undef ADVANCE_PC
#undef GOTO_NEXT
#undef DISPATCH
#undef DISPATCH_OFFSET
#undef GET_FRAME
#undef GET_ENTRY_FRAME
#undef SAVE_PC
#undef SAVE_ACC
#undef RESTORE_ACC
#undef INTERPRETER_GOTO_EXCEPTION_HANDLER
#undef CHECK_SWITCH_TO_DEBUGGER_TABLE
#undef REAL_GOTO_DISPATCH_OPCODE
#undef REAL_GOTO_EXCEPTION_HANDLER
#undef INTERPRETER_RETURN_IF_ABRUPT
#undef NOTIFY_DEBUGGER_EVENT
#undef DEPRECATED_CALL_INITIALIZE
#undef CALL_PUSH_UNDEFINED
#undef DEPRECATED_CALL_PUSH_ARGS_0
#undef DEPRECATED_CALL_PUSH_ARGS_1
#undef DEPRECATED_CALL_PUSH_ARGS_2
#undef DEPRECATED_CALL_PUSH_ARGS_3
#undef DEPRECATED_CALL_PUSH_ARGS_RANGE
#undef DEPRECATED_CALL_PUSH_ARGS_THISRANGE
#undef DEPRECATED_CALL_PUSH_ARGS_0_NO_EXTRA
#undef DEPRECATED_CALL_PUSH_ARGS_1_NO_EXTRA
#undef DEPRECATED_CALL_PUSH_ARGS_2_NO_EXTRA
#undef DEPRECATED_CALL_PUSH_ARGS_3_NO_EXTRA
#undef DEPRECATED_CALL_PUSH_ARGS_RANGE_NO_EXTRA
#undef DEPRECATED_CALL_PUSH_ARGS_THISRANGE_NO_EXTRA
#undef DEPRECATED_CALL_PUSH_ARGS
#undef UPDATE_HOTNESS_COUNTER_NON_ACC
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
#endif  // ECMASCRIPT_INTERPRETER_INTERPRETER_INL_H
