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

#include "ecmascript/dfx/vmstat/runtime_stat.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_runtime_stub-inl.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/template_string.h"

#include "libpandafile/code_data_accessor.h"
#include "libpandafile/file.h"
#include "libpandafile/method_data_accessor-inl.h"

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
#define READ_INST_8(offset) (*(pc + (offset)))
#define MOVE_AND_READ_INST_8(currentInst, offset) \
    (currentInst) <<= 8;                          \
    (currentInst) += READ_INST_8(offset);         \

#define READ_INST_16_0() READ_INST_16(2)
#define READ_INST_16_1() READ_INST_16(3)
#define READ_INST_16_2() READ_INST_16(4)
#define READ_INST_16_3() READ_INST_16(5)
#define READ_INST_16_5() READ_INST_16(7)
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
    JSTaggedType *prevSp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
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
    // check is or not debugger
    thread->CheckSwitchDebuggerBCStub();
#if ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER
    CpuProfiler *profiler = thread->GetEcmaVM()->GetProfiler();
    if (profiler != nullptr) {
        profiler->IsNeedAndGetStack(thread);
    }
#endif
    thread->CheckSafepoint();
    uint32_t argc = info->GetArgsNumber();
    uintptr_t argv = reinterpret_cast<uintptr_t>(info->GetArgs());
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_AsmInterpreterEntry);

    ECMAObject *callTarget = reinterpret_cast<ECMAObject*>(info->GetFunctionValue().GetTaggedObject());
    Method *method = callTarget->GetCallTarget();
    auto acc = reinterpret_cast<InterpreterEntry>(entry)(thread->GetGlueAddr(),
        callTarget, method, method->GetCallField(), argc, argv);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    ASSERT(FrameHandler::GetFrameType(sp) == FrameType::INTERPRETER_ENTRY_FRAME);
    auto prevEntry = InterpretedEntryFrame::GetFrameFromSp(sp)->GetPrevFrameFp();
    thread->SetCurrentSPFrame(prevEntry);

#if ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER
    if (profiler != nullptr) {
        profiler->IsNeedAndGetStack(thread);
    }
#endif
    return JSTaggedValue(acc);
}

JSTaggedValue InterpreterAssembly::GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context)
{
    // check is or not debugger
    thread->CheckSwitchDebuggerBCStub();
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_GeneratorReEnterAsmInterp);
    auto acc = reinterpret_cast<GeneratorReEnterInterpEntry>(entry)(thread->GetGlueAddr(), context.GetTaggedType());
    return JSTaggedValue(acc);
}

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
    SET_ACC(ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId));
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
    if (GET_ACC() == JSTaggedValue::False() || (GET_ACC().IsInt() && GET_ACC().GetInt() == 0) ||
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
    if (GET_ACC() == JSTaggedValue::False() || (GET_ACC().IsInt() && GET_ACC().GetInt() == 0) ||
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
    if (GET_ACC() == JSTaggedValue::True() || (GET_ACC().IsInt() && GET_ACC().GetInt() != 0) ||
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
    if (GET_ACC() == JSTaggedValue::True() || (GET_ACC().IsInt() && GET_ACC().GetInt() != 0) ||
        (GET_ACC().IsDouble() && GET_ACC().GetDouble() != 0)) {
        UPDATE_HOTNESS_COUNTER(offset);
        DISPATCH_OFFSET(offset);
    } else {
        DISPATCH(JNEZ_IMM16);
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
    auto imm = bit_cast<double>(READ_INST_64_0());
    LOG_INST() << "fldai " << imm;
    SET_ACC(JSTaggedValue(imm));
    DISPATCH(FLDAI_IMM64);
}

// void InterpreterAssembly::HandleCallarg0Imm8V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint32_t actualNumArgs = ActualNumArgsOfCall::CALLARG0;
//     uint32_t funcReg = READ_INST_8_1();
//     LOG_INST() << "callarg0 "
//                << "v" << funcReg;
//     // slow path
//     JSHandle<JSTaggedValue> funcHandle(thread, GET_VREG_VALUE(funcReg));
//     JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
//     EcmaRuntimeCallInfo *info =
//         EcmaInterpreter::NewRuntimeCallInfo(thread, funcHandle, undefined, undefined, actualNumArgs);
//     JSFunction::Call(info);
//     DISPATCH(CALLARG0_IMM8_V8);
// }

// void InterpreterAssembly::HandleCallarg1Imm8V8V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint32_t actualNumArgs = ActualNumArgsOfCall::CALLARG1;
//     uint32_t funcReg = READ_INST_8_1();
//     uint8_t a0 = READ_INST_8_2();
//     LOG_INST() << "callarg1 "
//                << "v" << funcReg << ", v" << a0;
//     // slow path
//     JSHandle<JSTaggedValue> funcHandle(thread, GET_VREG_VALUE(funcReg));
//     JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
//     EcmaRuntimeCallInfo *info =
//         EcmaInterpreter::NewRuntimeCallInfo(thread, funcHandle, undefined, undefined, actualNumArgs);
//     RETURN_IF_ABRUPT_COMPLETION(thread);
//     info->SetCallArg(GET_VREG_VALUE(a0));
//     JSFunction::Call(info);
//     DISPATCH(CALLARG1_IMM8_V8_V8);
// }

// void InterpreterAssembly::HandleCallargs2Imm8V8V8V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint32_t actualNumArgs = ActualNumArgsOfCall::CALLARGS2;
//     uint32_t funcReg = READ_INST_8_1();
//     uint8_t a0 = READ_INST_8_2();
//     uint8_t a1 = READ_INST_8_3();
//     LOG_INST() << "callargs2 "
//                << "v" << funcReg << ", v" << a0 << ", v" << a1;
//     // slow path
//     JSHandle<JSTaggedValue> funcHandle(thread, GET_VREG_VALUE(funcReg));
//     JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
//     EcmaRuntimeCallInfo *info =
//         EcmaInterpreter::NewRuntimeCallInfo(thread, funcHandle, undefined, undefined, actualNumArgs);
//     RETURN_IF_ABRUPT_COMPLETION(thread);
//     info->SetCallArg(GET_VREG_VALUE(a0), GET_VREG_VALUE(a1));
//     JSFunction::Call(info);
//     DISPATCH(CALLARGS2_IMM8_V8_V8_V8);
// }

// void InterpreterAssembly::HandleCallargs3Imm8V8V8V8V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint32_t actualNumArgs = ActualNumArgsOfCall::CALLARGS3;
//     uint32_t funcReg = READ_INST_8_1();
//     uint8_t a0 = READ_INST_8_2();
//     uint8_t a1 = READ_INST_8_3();
//     uint8_t a2 = READ_INST_8_4();
//     LOG_INST() << "callargs3 "
//                 << "v" << funcReg << ", v" << a0 << ", v" << a1 << ", v" << a2;
//     // slow path
//     JSHandle<JSTaggedValue> funcHandle(thread, GET_VREG_VALUE(funcReg));
//     JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
//     EcmaRuntimeCallInfo *info =
//         EcmaInterpreter::NewRuntimeCallInfo(thread, funcHandle, undefined, undefined, actualNumArgs);
//     RETURN_IF_ABRUPT_COMPLETION(thread);
//     info->SetCallArg(GET_VREG_VALUE(a0), GET_VREG_VALUE(a1), GET_VREG_VALUE(a2));
//     JSFunction::Call(info);
//     DISPATCH(CALLARGS3_IMM8_V8_V8_V8_V8);
// }

// void InterpreterAssembly::HandleCallthisrangeImm8Imm16V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint32_t actualNumArgs = READ_INST_16_1() - 1;  // 1: exclude this
//     uint32_t funcReg = READ_INST_8_3();
//     LOG_INST() << "calli.this.range " << actualNumArgs << ", v" << funcReg;
//     // slow path
//     JSHandle<JSTaggedValue> funcHandle(thread, GET_VREG_VALUE(funcReg));
//     JSHandle<JSTaggedValue> thisHandle(thread, GET_VREG_VALUE(funcReg + 1));
//     JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
//     EcmaRuntimeCallInfo *info =
//         EcmaInterpreter::NewRuntimeCallInfo(thread, funcHandle, thisHandle, undefined, actualNumArgs);
//     RETURN_IF_ABRUPT_COMPLETION(thread);
//     for (uint32_t i = 0; i < actualNumArgs; i++) {
//         info->SetCallArg(i, GET_VREG_VALUE(funcReg + i + 2));  // 2: start index of the first arg
//     }
//     JSFunction::Call(info);
//     DISPATCH(CALLTHISRANGE_IMM8_IMM16_V8);
// }

// void InterpreterAssembly::HandleCallspreadImm8V8V8V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint16_t v0 = READ_INST_8_1();
//     uint16_t v1 = READ_INST_8_2();
//     uint16_t v2 = READ_INST_8_3();
//     LOG_INST() << "intrinsics::callspread"
//                << " v" << v0 << " v" << v1 << " v" << v2;
//     JSTaggedValue func = GET_VREG_VALUE(v0);
//     JSTaggedValue obj = GET_VREG_VALUE(v1);
//     JSTaggedValue array = GET_VREG_VALUE(v2);

//     JSTaggedValue res = SlowRuntimeStub::CallSpread(thread, func, obj, array);
//     INTERPRETER_RETURN_IF_ABRUPT(res);
//     SET_ACC(res);

//     DISPATCH(CALLSPREAD_IMM8_V8_V8_V8);
// }

// void InterpreterAssembly::HandleCallrangeImm8Imm16V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint32_t actualNumArgs = READ_INST_16_1();
//     uint32_t funcReg = READ_INST_8_3();
//     LOG_INST() << "calli.range " << actualNumArgs << ", v" << funcReg;
//     // slow path
//     JSHandle<JSTaggedValue> funcHandle(thread, GET_VREG_VALUE(funcReg));
//     JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
//     EcmaRuntimeCallInfo *info =
//         EcmaInterpreter::NewRuntimeCallInfo(thread, funcHandle, undefined, undefined, actualNumArgs);
//     RETURN_IF_ABRUPT_COMPLETION(thread);
//     for (uint32_t i = 0; i < actualNumArgs; i++) {
//         info->SetCallArg(i, GET_VREG_VALUE(funcReg + i + 1));  // 1: start index of the first arg
//     }
//     JSFunction::Call(info);
//     DISPATCH(CALLRANGE_IMM8_IMM16_V8);
// }

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
    thread->SetCurrentSPFrame(sp);
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
    thread->SetCurrentSPFrame(sp);
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

void InterpreterAssembly::HandleLdglobalthis(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldglobalthis";
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    SET_ACC(globalObj);
    DISPATCH(LDGLOBALTHIS);
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

// TODO: Deprecated
// void InterpreterAssembly::HandleLdlexenv(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     LOG_INST() << "intrinsics::ldlexenv ";
//     AsmInterpretedFrame *state = GET_ASM_FRAME(sp);
//     JSTaggedValue currentLexenv = state->env;
//     SET_ACC(currentLexenv);
//     DISPATCH(LDLEXENV);
// }

void InterpreterAssembly::HandleGetunmappedargs(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::getunmappedargs";

    uint32_t startIdx = 0;
    uint32_t actualNumArgs = GetNumArgs(sp, 0, startIdx);

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
    JSTaggedValue res = SlowRuntimeStub::GetIterator(thread, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETITERATOR_IMM8);
}

void InterpreterAssembly::HandleThrowConstassignmentPrefV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "throwconstassignment"
                << " v" << v0;
    SlowRuntimeStub::ThrowConstAssignment(thread, GET_VREG_VALUE(v0));
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleThrowthrownotexists(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "throwthrownotexists";

    SlowRuntimeStub::ThrowThrowNotExists(thread);
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

void InterpreterAssembly::HandleThrowPatternnoncoerciblePrefNone(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "throwpatternnoncoercible";

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
    SlowRuntimeStub::ThrowIfNotObject(thread);
    INTERPRETER_GOTO_EXCEPTION_HANDLER();
}

// void InterpreterAssembly::HandleIternextImm8V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint16_t v0 = READ_INST_8_1();
//     LOG_INST() << "intrinsics::iternext"
//                << " v" << v0;
//     JSTaggedValue iter = GET_VREG_VALUE(v0);
//     JSTaggedValue res = SlowRuntimeStub::IterNext(thread, iter);
//     INTERPRETER_RETURN_IF_ABRUPT(res);
//     SET_ACC(res);
//     DISPATCH(ITERNEXT_IMM8_V8);
// }

void InterpreterAssembly::HandleCloseiteratorImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsics::closeiterator"
               << " v" << v0;
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
    int32_t opNumber0;
    int32_t opNumber1;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        opNumber0 = left.GetInt();
        opNumber1 = right.GetInt();
    } else if (left.IsNumber() && right.IsNumber()) {
        opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
    } else {
        // slow path
        SAVE_ACC();
        JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, left);
        INTERPRETER_RETURN_IF_ABRUPT(taggedNumber0);
        RESTORE_ACC();
        right = GET_ACC();  // Maybe moved by GC
        JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithUint32(thread, right);
        INTERPRETER_RETURN_IF_ABRUPT(taggedNumber1);
        opNumber0 = taggedNumber0.GetInt();
        opNumber1 = taggedNumber1.GetInt();
    }

    uint32_t shift =
        static_cast<uint32_t>(opNumber1) & 0x1f;  // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
    using unsigned_type = std::make_unsigned_t<int32_t>;
    auto ret =
        static_cast<int32_t>(static_cast<unsigned_type>(opNumber0) << shift);  // NOLINT(hicpp-signed-bitwise)
    SET_ACC(JSTaggedValue(ret));
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
    int32_t opNumber0;
    int32_t opNumber1;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        opNumber0 = left.GetInt();
        opNumber1 = right.GetInt();
    } else if (left.IsNumber() && right.IsNumber()) {
        opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
    } else {
        // slow path
        SAVE_ACC();
        JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, left);
        INTERPRETER_RETURN_IF_ABRUPT(taggedNumber0);
        RESTORE_ACC();
        right = GET_ACC();  // Maybe moved by GC
        JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, right);
        INTERPRETER_RETURN_IF_ABRUPT(taggedNumber1);
        opNumber0 = taggedNumber0.GetInt();
        opNumber1 = taggedNumber1.GetInt();
    }
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) & static_cast<uint32_t>(opNumber1);
    SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    DISPATCH(AND2_IMM8_V8);
}

void InterpreterAssembly::HandleOr2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::or2"
               << " v" << v0;
    int32_t opNumber0;
    int32_t opNumber1;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        opNumber0 = left.GetInt();
        opNumber1 = right.GetInt();
    } else if (left.IsNumber() && right.IsNumber()) {
        opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
    } else {
        // slow path
        SAVE_ACC();
        JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, left);
        INTERPRETER_RETURN_IF_ABRUPT(taggedNumber0);
        RESTORE_ACC();
        right = GET_ACC();  // Maybe moved by GC
        JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, right);
        INTERPRETER_RETURN_IF_ABRUPT(taggedNumber1);
        opNumber0 = taggedNumber0.GetInt();
        opNumber1 = taggedNumber1.GetInt();
    }
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) | static_cast<uint32_t>(opNumber1);
    SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    DISPATCH(OR2_IMM8_V8);
}

void InterpreterAssembly::HandleXor2Imm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();

    LOG_INST() << "intrinsics::xor2"
               << " v" << v0;
    int32_t opNumber0;
    int32_t opNumber1;
    JSTaggedValue left = GET_VREG_VALUE(v0);
    JSTaggedValue right = GET_ACC();
    // both number, fast path
    if (left.IsInt() && right.IsInt()) {
        opNumber0 = left.GetInt();
        opNumber1 = right.GetInt();
    } else if (left.IsNumber() && right.IsNumber()) {
        opNumber0 =
            left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
        opNumber1 =
            right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
    } else {
        // slow path
        SAVE_ACC();
        JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, left);
        INTERPRETER_RETURN_IF_ABRUPT(taggedNumber0);
        RESTORE_ACC();
        right = GET_ACC();  // Maybe moved by GC
        JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, right);
        INTERPRETER_RETURN_IF_ABRUPT(taggedNumber1);
        opNumber0 = taggedNumber0.GetInt();
        opNumber1 = taggedNumber1.GetInt();
    }
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) ^ static_cast<uint32_t>(opNumber1);
    SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)));
    DISPATCH(XOR2_IMM8_V8);
}

void InterpreterAssembly::HandleDelobjpropV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    uint16_t v1 = READ_INST_8_1();
    LOG_INST() << "intrinsics::delobjprop"
               << " v0" << v0 << " v1" << v1;

    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue prop = GET_VREG_VALUE(v1);
    JSTaggedValue res = SlowRuntimeStub::DelObjProp(thread, obj, prop);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);

    DISPATCH(DELOBJPROP_V8_V8);
}

void InterpreterAssembly::HandleDefinefuncImm8Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_16_3();
    uint16_t v0 = READ_INST_8_5();
    LOG_INST() << "intrinsics::definefunc length: " << length
               << " v" << v0;
    JSFunction *result = JSFunction::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(methodId).GetTaggedObject());
    ASSERT(result != nullptr);
    if (result->GetResolved()) {
        auto res = SlowRuntimeStub::Definefunc(thread, result);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        result = JSFunction::Cast(res.GetTaggedObject());
    } else {
        result->SetResolved(thread);
    }

    result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
    JSTaggedValue envHandle = GET_VREG_VALUE(v0);
    result->SetLexicalEnv(thread, envHandle);

    JSFunction *currentFunc = JSFunction::Cast((GET_ASM_FRAME(sp)->function).GetTaggedObject());
    result->SetModule(thread, currentFunc->GetModule());
    SET_ACC(JSTaggedValue(result));

    DISPATCH(DEFINEFUNC_IMM8_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleDefinencfuncImm8Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_16_3();
    uint16_t v0 = READ_INST_8_5();
    JSTaggedValue homeObject = GET_ACC();
    LOG_INST() << "intrinsics::definencfunc length: " << length
               << " v" << v0;
    JSFunction *result = JSFunction::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(methodId).GetTaggedObject());
    ASSERT(result != nullptr);
    if (result->GetResolved()) {
        SAVE_ACC();
        auto res = SlowRuntimeStub::DefineNCFunc(thread, result);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        result = JSFunction::Cast(res.GetTaggedObject());
        RESTORE_ACC();
        homeObject = GET_ACC();
    } else {
        result->SetResolved(thread);
    }

    result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
    JSTaggedValue env = GET_VREG_VALUE(v0);
    result->SetLexicalEnv(thread, env);
    result->SetHomeObject(thread, homeObject);

    JSFunction *currentFunc = JSFunction::Cast((GET_ASM_FRAME(sp)->function).GetTaggedObject());
    result->SetModule(thread, currentFunc->GetModule());
    SET_ACC(JSTaggedValue(result));

    DISPATCH(DEFINENCFUNC_IMM8_ID16_IMM16_V8);
}

// void InterpreterAssembly::HandleDefinemethodImm8Id16Imm16V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint16_t methodId = READ_INST_16_1();
//     uint16_t length = READ_INST_16_3();
//     uint16_t v0 = READ_INST_8_5();
//     JSTaggedValue homeObject = GET_ACC();
//     LOG_INST() << "intrinsics::definemethod length: " << length
//                << " v" << v0;
//     JSFunction *result = JSFunction::Cast(
//         ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(methodId).GetTaggedObject());
//     ASSERT(result != nullptr);
//     if (result->GetResolved()) {
//         auto res = SlowRuntimeStub::DefineMethod(thread, result, homeObject);
//         INTERPRETER_RETURN_IF_ABRUPT(res);
//         result = JSFunction::Cast(res.GetTaggedObject());
//     } else {
//         result->SetHomeObject(thread, homeObject);
//         result->SetResolved(thread);
//     }

//     result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
//     JSTaggedValue taggedCurEnv = GET_VREG_VALUE(v0);
//     result->SetLexicalEnv(thread, taggedCurEnv);

//     JSFunction *currentFunc = JSFunction::Cast((GET_ASM_FRAME(sp)->function).GetTaggedObject());
//     result->SetModule(thread, currentFunc->GetModule());
//     SET_ACC(JSTaggedValue(result));

//     DISPATCH(DEFINEMETHOD_IMM8_ID16_IMM16_V8);
// }

// TODO
// void InterpreterAssembly::HandleNewobjrangeImm8Imm16V8(
//     JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
//     JSTaggedValue acc, int16_t hotnessCounter)
// {
//     uint16_t numArgs = READ_INST_16_1();
//     uint16_t firstArgRegIdx = READ_INST_8_3();
//     LOG_INST() << "intrinsics::newobjRange " << numArgs << " v" << firstArgRegIdx;
//     JSTaggedValue ctor = GET_VREG_VALUE(firstArgRegIdx);

//     // slow path
//     constexpr uint16_t firstArgOffset = 2;
//     JSTaggedValue newTarget = GET_VREG_VALUE(firstArgRegIdx + 1);
//     // Exclude func and newTarget
//     uint16_t firstArgIdx = firstArgRegIdx + firstArgOffset;
//     uint16_t length = numArgs - firstArgOffset;

//     SAVE_PC();
//     JSTaggedValue res = SlowRuntimeStub::NewObjRange(thread, ctor, newTarget, firstArgIdx, length);
//     INTERPRETER_RETURN_IF_ABRUPT(res);
//     SET_ACC(res);
//     DISPATCH(NEWOBJRANGE_IMM8_IMM16_V8);
// }

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
        if (std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) {
            SET_ACC(JSTaggedValue(base::NAN_VALUE));
        }
        if ((doubleBase == 0 &&
            ((bit_cast<uint64_t>(doubleBase)) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK) &&
            std::isfinite(doubleExponent) && base::NumberHelper::TruncateDouble(doubleExponent) == doubleExponent &&
            base::NumberHelper::TruncateDouble(doubleExponent / 2) + base::HALF ==  // 2 : half
            (doubleExponent / 2)) {  // 2 : half
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
    bool res = FastRuntimeStub::FastStrictEqual(left, right);
    SET_ACC(JSTaggedValue(!res));
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
    bool res = FastRuntimeStub::FastStrictEqual(left, right);
    SET_ACC(JSTaggedValue(res));
    DISPATCH(STRICTEQ_IMM8_V8);
}

void InterpreterAssembly::HandleLdlexvarImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t level = READ_INST_16_0();
    uint16_t slot = READ_INST_16_2();

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
    DISPATCH(LDLEXVAR_IMM16_IMM16);
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
                << " v" << v0 << " v" << v1;
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
               << " v" << v0 << " v" << v1;
    JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_ACC();
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
    JSTaggedValue res = SlowRuntimeStub::NewObjApply(thread, func, array);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(NEWOBJSPREAD_IMM8_V8_V8);
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
        JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
        JSTaggedValue value = GET_ACC();
        // fast path
        SAVE_ACC();
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName<true>(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOWNBYNAME_IMM8_ID16_V8);
        }
        RESTORE_ACC();
    }
    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
    auto propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);  // Maybe moved by GC
    auto value = GET_ACC();                                  // Maybe moved by GC
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
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateEmptyObject(thread, factory, globalEnv);
    SET_ACC(res);
    DISPATCH(CREATEEMPTYOBJECT);
}

void InterpreterAssembly::HandleCreateobjectwithbufferImm8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    LOG_INST() << "intrinsics::createobjectwithbuffer"
               << " imm:" << imm;
    JSObject *result = JSObject::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(imm).GetTaggedObject());
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateObjectWithBuffer(thread, factory, result);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEOBJECTWITHBUFFER_IMM8_IMM16);
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
    JSTaggedValue res = SlowRuntimeStub::SetObjectWithProto(thread, proto, obj);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(SETOBJECTWITHPROTO_IMM8_V8);
}

void InterpreterAssembly::HandleCreatearraywithbufferImm8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    LOG_INST() << "intrinsics::createarraywithbuffer"
               << " imm:" << imm;
    JSArray *result = JSArray::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(imm).GetTaggedObject());
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateArrayWithBuffer(thread, factory, result);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEARRAYWITHBUFFER_IMM8_IMM16);
}

void InterpreterAssembly::HandleGetmodulenamespaceId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    auto localName = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);

    LOG_INST() << "intrinsics::getmodulenamespace "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(localName.GetTaggedObject()));

    JSTaggedValue moduleNamespace = SlowRuntimeStub::GetModuleNamespace(thread, localName);
    INTERPRETER_RETURN_IF_ABRUPT(moduleNamespace);
    SET_ACC(moduleNamespace);
    DISPATCH(GETMODULENAMESPACE_ID16);
}

void InterpreterAssembly::HandleStmodulevarId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    auto key = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);

    LOG_INST() << "intrinsics::stmodulevar "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(key.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();

    SAVE_ACC();
    SlowRuntimeStub::StModuleVar(thread, key, value);
    RESTORE_ACC();
    DISPATCH(STMODULEVAR_ID16);
}

void InterpreterAssembly::HandleLdmodulevarId16Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    uint8_t innerFlag = READ_INST_8_2();

    JSTaggedValue key = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    LOG_INST() << "intrinsics::ldmodulevar "
               << "string_id:" << stringId << ", "
               << "key: " << ConvertToString(EcmaString::Cast(key.GetTaggedObject()));

    JSTaggedValue moduleVar = SlowRuntimeStub::LdModuleVar(thread, key, innerFlag != 0);
    INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
    SET_ACC(moduleVar);
    DISPATCH(LDMODULEVAR_ID16_IMM8);
}

void InterpreterAssembly::HandleCreateregexpwithliteralImm8Id16Imm8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    JSTaggedValue pattern = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    uint8_t flags = READ_INST_8_3();
    LOG_INST() << "intrinsics::createregexpwithliteral "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(pattern.GetTaggedObject()))
               << ", flags:" << flags;
    JSTaggedValue res = SlowRuntimeStub::CreateRegExpWithLiteral(thread, pattern, flags);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8);
}

void InterpreterAssembly::HandleGettemplateobjectImm8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
    LOG_INST() << "intrinsic::gettemplateobject"
               << " v" << v0;

    JSTaggedValue literal = GET_VREG_VALUE(v0);
    JSTaggedValue res = SlowRuntimeStub::GetTemplateObject(thread, literal);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETTEMPLATEOBJECT_IMM8_V8);
}

void InterpreterAssembly::HandleGetnextpropnameV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsic::getnextpropname"
                << " v" << v0;
    JSTaggedValue iter = GET_VREG_VALUE(v0);
    JSTaggedValue res = SlowRuntimeStub::GetNextPropName(thread, iter);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETNEXTPROPNAME_V8);
}

void InterpreterAssembly::HandleCopydatapropertiesV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    uint16_t v1 = READ_INST_8_1();
    LOG_INST() << "intrinsic::copydataproperties"
               << " v" << v0 << " v" << v1;
    JSTaggedValue dst = GET_VREG_VALUE(v0);
    JSTaggedValue src = GET_VREG_VALUE(v1);
    JSTaggedValue res = SlowRuntimeStub::CopyDataProperties(thread, dst, src);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(COPYDATAPROPERTIES_V8_V8);
}

void InterpreterAssembly::HandleStownbyindexImm8V8Imm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint32_t index = READ_INST_32_2();
    LOG_INST() << "intrinsics::stownbyindex"
               << " v" << v0 << " imm" << index;
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    // fast path
    if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
        SAVE_ACC();
        JSTaggedValue value = GET_ACC();
        // fast path
        JSTaggedValue res =
            FastRuntimeStub::SetPropertyByIndex<true>(thread, receiver, index, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOWNBYINDEX_IMM8_V8_IMM32);
        }
        RESTORE_ACC();
    }
    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
    auto value = GET_ACC();         // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StOwnByIndex(thread, receiver, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOWNBYINDEX_IMM8_V8_IMM32);
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
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue<true>(thread, receiver, propKey, value);

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

    JSTaggedValue res = SlowRuntimeStub::CreateObjectWithExcludedKeys(thread, numKeys, obj, firstArgRegIdx);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8);
}

void InterpreterAssembly::HandleDefinegeneratorfuncImm8Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_16_3();
    uint16_t v0 = READ_INST_8_5();
    LOG_INST() << "define gengerator function length: " << length
               << " v" << v0;
    JSFunction *result = JSFunction::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(methodId).GetTaggedObject());
    ASSERT(result != nullptr);
    if (result->GetResolved()) {
        auto res = SlowRuntimeStub::DefineGeneratorFunc(thread, result);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        result = JSFunction::Cast(res.GetTaggedObject());
    } else {
        result->SetResolved(thread);
    }

    result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
    JSTaggedValue env = GET_VREG_VALUE(v0);
    result->SetLexicalEnv(thread, env);

    JSFunction *currentFunc = JSFunction::Cast((GET_ASM_FRAME(sp)->function).GetTaggedObject());
    result->SetModule(thread, currentFunc->GetModule());
    SET_ACC(JSTaggedValue(result));
    DISPATCH(DEFINEGENERATORFUNC_IMM8_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleDefineasyncgeneratorfuncImm8Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_16_3();
    uint16_t v0 = READ_INST_8_5();
    LOG_INST() << "define async gengerator function length: " << length
               << " v" << v0;
    JSFunction *result = JSFunction::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(methodId).GetTaggedObject());
    ASSERT(result != nullptr);
    if (result->GetResolved()) {
        auto res = SlowRuntimeStub::DefineAsyncGeneratorFunc(thread, result);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        result = JSFunction::Cast(res.GetTaggedObject());
    } else {
        result->SetResolved(thread);
    }

    result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
    JSTaggedValue env = GET_VREG_VALUE(v0);
    result->SetLexicalEnv(thread, env);

    JSFunction *currentFunc = JSFunction::Cast((GET_ASM_FRAME(sp)->function).GetTaggedObject());
    result->SetModule(thread, currentFunc->GetModule());
    SET_ACC(JSTaggedValue(result));
    DISPATCH(DEFINEASYNCGENERATORFUNC_IMM8_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleDefineasyncfuncImm8Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_16_3();
    uint16_t v0 = READ_INST_8_5();
    LOG_INST() << "define async function length: " << length
               << " v" << v0;
    JSFunction *result = JSFunction::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(methodId).GetTaggedObject());
    ASSERT(result != nullptr);
    if (result->GetResolved()) {
        auto res = SlowRuntimeStub::DefineAsyncFunc(thread, result);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        result = JSFunction::Cast(res.GetTaggedObject());
    } else {
        result->SetResolved(thread);
    }

    result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
    JSTaggedValue env = GET_VREG_VALUE(v0);
    result->SetLexicalEnv(thread, env);

    JSFunction *currentFunc = JSFunction::Cast((GET_ASM_FRAME(sp)->function).GetTaggedObject());
    result->SetModule(thread, currentFunc->GetModule());
    SET_ACC(JSTaggedValue(result));
    DISPATCH(DEFINEASYNCFUNC_IMM8_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleLdhole(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsic::ldhole";
    SET_ACC(JSTaggedValue::Hole());
    DISPATCH(LDHOLE);
}

void InterpreterAssembly::HandleCopyrestargsImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t restIdx = READ_INST_16_0();
    LOG_INST() << "intrinsics::copyrestargs"
               << " index: " << restIdx;

    uint32_t startIdx = 0;
    uint32_t restNumArgs = GetNumArgs(sp, restIdx, startIdx);

    JSTaggedValue res = SlowRuntimeStub::CopyRestArgs(thread, sp, restNumArgs, startIdx);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(COPYRESTARGS_IMM16);
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
    JSTaggedValue res =
        SlowRuntimeStub::DefineGetterSetterByValue(thread, obj, prop, getter, setter, flag.ToBoolean());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8);
}

void InterpreterAssembly::HandleLdobjbyindexImm8V8Imm32(
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
            DISPATCH(LDOBJBYINDEX_IMM8_V8_IMM32);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    JSTaggedValue res = SlowRuntimeStub::LdObjByIndex(thread, receiver, idx, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYINDEX_IMM8_V8_IMM32);
}

void InterpreterAssembly::HandleStobjbyindexImm8V8Imm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_1();
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
            DISPATCH(STOBJBYINDEX_IMM8_V8_IMM32);
        }
        RESTORE_ACC();
    }
    // slow path
    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);    // Maybe moved by GC
    JSTaggedValue value = GET_ACC();  // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StObjByIndex(thread, receiver, index, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOBJBYINDEX_IMM8_V8_IMM32);
}

void InterpreterAssembly::HandleLdobjbyvalueImm8V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_1();
    uint32_t v1 = READ_INST_8_2();
    LOG_INST() << "intrinsics::Ldobjbyvalue"
                << " v" << v0 << " v" << v1;

    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = GET_VREG_VALUE(v1);

#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByValue(thread, receiver, propKey, firstValue, secondValue);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            res = ICRuntimeStub::LoadICByValue(thread, profileTypeArray, receiver, propKey, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYVALUE_IMM8_V8_V8);
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
            DISPATCH(LDOBJBYVALUE_IMM8_V8_V8);
        }
    }
    // slow path
    JSTaggedValue res = SlowRuntimeStub::LdObjByValue(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYVALUE_IMM8_V8_V8);
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
        receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
        JSTaggedValue propKey = GET_VREG_VALUE(v1);   // Maybe moved by GC
        JSTaggedValue value = GET_ACC();              // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StObjByValue(thread, receiver, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    }
    DISPATCH(STOBJBYVALUE_IMM8_V8_V8);
}

void InterpreterAssembly::HandleLdsuperbyvalueImm8V8V8(
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
    JSTaggedValue thisFunc = GetThisFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, receiver, propKey, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDSUPERBYVALUE_IMM8_V8_V8);
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
    JSTaggedValue thisFunc = GetThisFunction(sp);
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
    auto prop = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);

    LOG_INST() << "intrinsics::tryldglobalbyname "
                << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(prop.GetTaggedObject()));

#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        EcmaVM *ecmaVm = thread->GetEcmaVM();
        JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
        JSTaggedValue globalObj = globalEnv->GetGlobalObject();
        JSTaggedValue res = ICRuntimeStub::LoadGlobalICByName(thread,
                                                              ProfileTypeInfo::Cast(
                                                                  profileTypeInfo.GetTaggedObject()),
                                                              globalObj, prop, slotId);
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
        EcmaVM *ecmaVm = thread->GetEcmaVM();
        JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
        JSTaggedValue globalObj = globalEnv->GetGlobalObject();
        JSTaggedValue globalResult = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, prop);
        if (!globalResult.IsHole()) {
            SET_ACC(globalResult);
        } else {
            // slow path
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
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    LOG_INST() << "intrinsics::trystglobalbyname"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        EcmaVM *ecmaVm = thread->GetEcmaVM();
        JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
        JSTaggedValue globalObj = globalEnv->GetGlobalObject();
        JSTaggedValue res = ICRuntimeStub::StoreGlobalICByName(thread,
                                                               ProfileTypeInfo::Cast(
                                                                   profileTypeInfo.GetTaggedObject()),
                                                               globalObj, propKey, value, slotId);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(TRYSTGLOBALBYNAME_IMM8_ID16);
    }
#endif

    auto recordResult = SlowRuntimeStub::LdGlobalRecord(thread, propKey);
    // 1. find from global record
    if (!recordResult.IsUndefined()) {
        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        JSTaggedValue res = SlowRuntimeStub::TryUpdateGlobalRecord(thread, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
    } else {
        // 2. find from global object
        EcmaVM *ecmaVm = thread->GetEcmaVM();
        JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
        JSTaggedValue globalObj = globalEnv->GetGlobalObject();
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

void InterpreterAssembly::HandleStconsttoglobalrecordId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    LOG_INST() << "intrinsics::stconsttoglobalrecord"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();
    SAVE_ACC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, true);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STCONSTTOGLOBALRECORD_ID16);
}

void InterpreterAssembly::HandleStlettoglobalrecordId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    LOG_INST() << "intrinsics::stlettoglobalrecord"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();
    SAVE_ACC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, false);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STLETTOGLOBALRECORD_ID16);
}

void InterpreterAssembly::HandleStclasstoglobalrecordId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    LOG_INST() << "intrinsics::stclasstoglobalrecord"
               << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

    JSTaggedValue value = GET_ACC();
    SAVE_ACC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, false);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STCLASSTOGLOBALRECORD_ID16);
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
        JSTaggedValue res = FastRuntimeStub::SetPropertyByValue<true>(thread, receiver, propKey, value);

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
        JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
        JSTaggedValue value = GET_ACC();
        // fast path
        SAVE_ACC();
        JSTaggedValue res = FastRuntimeStub::SetPropertyByName<true>(thread, receiver, propKey, value);
        if (!res.IsHole()) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            JSFunction::SetFunctionNameNoPrefix(thread, JSFunction::Cast(value.GetTaggedObject()), propKey);
            RESTORE_ACC();
            DISPATCH(STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8);
        }
        RESTORE_ACC();
    }

    SAVE_ACC();
    receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
    auto propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);  // Maybe moved by GC
    auto value = GET_ACC();                                  // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StOwnByNameWithNameSet(thread, receiver, propKey, value);
    RESTORE_ACC();
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleLdglobalvarId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t stringId = READ_INST_16_0();
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();

    JSTaggedValue result = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, propKey);
    if (!result.IsHole()) {
        SET_ACC(result);
    } else {
        // slow path
        JSTaggedValue res = SlowRuntimeStub::LdGlobalVarFromGlobalProto(thread, globalObj, propKey);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(LDGLOBALVAR_ID16);
}

void InterpreterAssembly::HandleLdobjbynameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_3();
    JSTaggedValue receiver = GET_VREG_VALUE(v0);

#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryLoadICByName(thread, receiver, firstValue, secondValue);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            uint16_t stringId = READ_INST_16_1();
            JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
            res = ICRuntimeStub::LoadICByName(thread, profileTypeArray, receiver, propKey, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(LDOBJBYNAME_IMM8_ID16_V8);
        }
    }
#endif
    uint16_t stringId = READ_INST_16_1();
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
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
            DISPATCH(LDOBJBYNAME_IMM8_ID16_V8);
        }
    }
    // not meet fast condition or fast path return hole, walk slow path
    // slow stub not need receiver
    JSTaggedValue res = SlowRuntimeStub::LdObjByName(thread, receiver, propKey, false, JSTaggedValue::Undefined());
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDOBJBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleStobjbynameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint32_t v0 = READ_INST_8_3();
    JSTaggedValue receiver = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_ACC();
#if ECMASCRIPT_ENABLE_IC
    if (!profileTypeInfo.IsUndefined()) {
        uint16_t slotId = READ_INST_8_0();
        auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
        JSTaggedValue firstValue = profileTypeArray->Get(slotId);
        JSTaggedValue res = JSTaggedValue::Hole();
        SAVE_ACC();

        if (LIKELY(firstValue.IsHeapObject())) {
            JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
            res = ICRuntimeStub::TryStoreICByName(thread, receiver, firstValue, secondValue, value);
        }
        // IC miss and not enter the megamorphic state, store as polymorphic
        if (res.IsHole() && !firstValue.IsHole()) {
            uint16_t stringId = READ_INST_16_1();
            JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
            res = ICRuntimeStub::StoreICByName(thread, profileTypeArray, receiver, propKey, value, slotId);
        }

        if (LIKELY(!res.IsHole())) {
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(STOBJBYNAME_IMM8_ID16_V8);
        }
    }
#endif
    uint16_t stringId = READ_INST_16_1();
    LOG_INST() << "intrinsics::stobjbyname "
                << "v" << v0 << " stringId:" << stringId;
    if (receiver.IsHeapObject()) {
        JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
        value = GET_ACC();
        // fast path
        SAVE_ACC();
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
    receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
    auto propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);  // Maybe moved by GC
    value = GET_ACC();                                  // Maybe moved by GC
    JSTaggedValue res = SlowRuntimeStub::StObjByName(thread, receiver, propKey, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STOBJBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleLdsuperbynameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    uint32_t v0 = READ_INST_8_3();
    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);

    LOG_INST() << "intrinsics::ldsuperbyname"
               << "v" << v0 << " stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData();

    JSTaggedValue thisFunc = GetThisFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, obj, propKey, thisFunc);

    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(LDSUPERBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleStsuperbynameImm8Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_1();
    uint32_t v0 = READ_INST_8_3();

    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue propKey = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    JSTaggedValue value = GET_ACC();

    LOG_INST() << "intrinsics::stsuperbyname"
               << "v" << v0 << " stringId:" << stringId << ", "
               << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData()
               << ", value:" << value.GetRawData();

    // slow path
    SAVE_ACC();
    JSTaggedValue thisFunc = GetThisFunction(sp);
    JSTaggedValue res = SlowRuntimeStub::StSuperByValue(thread, obj, propKey, value, thisFunc);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STSUPERBYNAME_IMM8_ID16_V8);
}

void InterpreterAssembly::HandleStglobalvarId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    JSTaggedValue prop = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(stringId);
    JSTaggedValue value = GET_ACC();

    LOG_INST() << "intrinsics::stglobalvar "
               << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(prop.GetTaggedObject()))
               << ", value:" << value.GetRawData();

    SAVE_ACC();
    JSTaggedValue res = SlowRuntimeStub::StGlobalVar(thread, prop, value);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    RESTORE_ACC();
    DISPATCH(STGLOBALVAR_ID16);
}

void InterpreterAssembly::HandleCreategeneratorobjV8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    LOG_INST() << "intrinsics::creategeneratorobj"
               << " v" << v0;
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
    LOG_INST() << "intrinsics::"
               << " v" << v0;
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
    LOG_INST() << "intrinsics::LowerAsyncGeneratorResolve"
               << " v" << v0;
    JSTaggedValue asyncGenerator = GET_VREG_VALUE(v0);
    JSTaggedValue value = GET_VREG_VALUE(v1);
    JSTaggedValue flag = GET_VREG_VALUE(v2);
    JSTaggedValue res = SlowRuntimeStub::AsyncGeneratorResolve(thread, asyncGenerator, value, flag);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(ASYNCGENERATORRESOLVE_V8_V8_V8);
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
    JSTaggedValue res = SlowRuntimeStub::StArraySpread(thread, dst, index, src);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(STARRAYSPREAD_V8_V8);
}

void InterpreterAssembly::HandleGetiteratornextV8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t v0 = READ_INST_8_0();
    uint16_t v1 = READ_INST_8_1();
    LOG_INST() << "intrinsic::getiteratornext"
               << " v" << v0 << " v" << v1;
    JSTaggedValue obj = GET_VREG_VALUE(v0);
    JSTaggedValue method = GET_VREG_VALUE(v1);
    JSTaggedValue res = SlowRuntimeStub::GetIteratorNext(thread, obj, method);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(GETITERATORNEXT_V8_V8);
}

void InterpreterAssembly::HandleDefineclasswithbufferImm8Id16Imm16Imm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t length = READ_INST_16_5();
    uint16_t v0 = READ_INST_8_7();
    uint16_t v1 = READ_INST_8_8();
    LOG_INST() << "intrinsics::defineclasswithbuffer"
                << " method id:" << methodId << " lexenv: v" << v0 << " parent: v" << v1;
    JSFunction *classTemplate = JSFunction::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(methodId).GetTaggedObject());
    ASSERT(classTemplate != nullptr);

    JSTaggedValue lexenv = GET_VREG_VALUE(v0);
    JSTaggedValue proto = GET_VREG_VALUE(v1);

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::CloneClassFromTemplate(thread, JSTaggedValue(classTemplate), proto, lexenv);

    INTERPRETER_RETURN_IF_ABRUPT(res);
    ASSERT(res.IsClassConstructor());
    JSFunction *cls = JSFunction::Cast(res.GetTaggedObject());

    lexenv = GET_VREG_VALUE(v0);  // slow runtime may gc
    cls->SetLexicalEnv(thread, lexenv);

    JSFunction *currentFunc = JSFunction::Cast((GET_ASM_FRAME(sp)->function).GetTaggedObject());
    cls->SetModule(thread, currentFunc->GetModule());

    SlowRuntimeStub::SetClassConstructorLength(thread, res, JSTaggedValue(length));

    SET_ACC(res);
    DISPATCH(DEFINECLASSWITHBUFFER_IMM8_ID16_IMM16_IMM16_V8_V8);
}

void InterpreterAssembly::HandleLdthisfunction(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsic::ldfunction";
    SET_ACC(GetThisFunction(sp));
    DISPATCH(LDFUNCTION);
}

void InterpreterAssembly::HandleNewlexenvwithnameImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t numVars = READ_INST_16_0();
    uint16_t scopeId = READ_INST_16_2();
    LOG_INST() << "intrinsics::newlexenvwithname"
               << " numVars " << numVars << " scopeId " << scopeId;

    SAVE_PC();
    JSTaggedValue res = SlowRuntimeStub::NewLexicalEnvWithName(thread, numVars, scopeId);
    INTERPRETER_RETURN_IF_ABRUPT(res);

    SET_ACC(res);
    GET_ASM_FRAME(sp)->env = res;
    DISPATCH(NEWLEXENVWITHNAME_IMM16_IMM16);
}

void InterpreterAssembly::HandleLdbigintId16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t stringId = READ_INST_16_0();
    LOG_INST() << "intrinsic::ldbigint";
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
        JSTaggedValue res = SlowRuntimeStub::ToNumeric(thread, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
    }
    DISPATCH(TONUMERIC_IMM8);
}

void InterpreterAssembly::HandleSupercallImm8Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t range = READ_INST_16_1();
    uint16_t v0 = READ_INST_8_3();
    LOG_INST() << "intrinsics::supercall"
                << " range: " << range << " v" << v0;

    JSTaggedValue thisFunc = GET_ACC();
    JSTaggedValue newTarget = GetNewTarget(sp);

    JSTaggedValue res = SlowRuntimeStub::SuperCall(thread, thisFunc, newTarget, v0, range);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(SUPERCALL_IMM8_IMM16_V8);
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

    JSTaggedValue res = SlowRuntimeStub::SuperCallSpread(thread, thisFunc, newTarget, array);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(SUPERCALLSPREAD_IMM8_V8);
}

void InterpreterAssembly::HandleCreateobjecthavingmethodImm8Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_1();
    LOG_INST() << "intrinsics::createobjecthavingmethod"
               << " imm:" << imm;
    JSObject *result = JSObject::Cast(
        ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(imm).GetTaggedObject());
    JSTaggedValue env = GET_ACC();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSTaggedValue res = SlowRuntimeStub::CreateObjectHavingMethod(thread, factory, result, env);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    SET_ACC(res);
    DISPATCH(CREATEOBJECTHAVINGMETHOD_IMM8_IMM16);
}

void InterpreterAssembly::HandleThrowifsupernotcorrectcallImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    uint16_t imm = READ_INST_16_0();
    JSTaggedValue thisValue = GET_ACC();
    LOG_INST() << "intrinsic::throwifsupernotcorrectcall"
               << " imm:" << imm;
    JSTaggedValue res = SlowRuntimeStub::ThrowIfSuperNotCorrectCall(thread, imm, thisValue);
    INTERPRETER_RETURN_IF_ABRUPT(res);
    DISPATCH(THROWIFSUPERNOTCORRECTCALL_IMM16);
}

void InterpreterAssembly::HandleLdhomeobject(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::ldhomeobject";

    JSTaggedValue thisFunc = GetThisFunction(sp);
    JSTaggedValue homeObject = JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject();

    SET_ACC(homeObject);
    DISPATCH(LDHOMEOBJECT);
}

void InterpreterAssembly::HandleThrowdeletesuperproperty(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "throwdeletesuperproperty";

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

void InterpreterAssembly::HandleIsfalse(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INST() << "intrinsics::isfalse";
    if (!GET_ACC().ToBoolean()) {
        SET_ACC(JSTaggedValue::True());
    } else {
        SET_ACC(JSTaggedValue::False());
    }
    DISPATCH(ISFALSE);
}

void InterpreterAssembly::HandleTypeofImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(TYPEOF_IMM16);
}

void InterpreterAssembly::HandleCreateemptyarrayImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CREATEEMPTYARRAY_IMM16);
}

void InterpreterAssembly::HandleGetiteratorImm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(GETITERATOR_IMM16);
}

void InterpreterAssembly::HandleGettemplateobjectImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(GETTEMPLATEOBJECT_IMM16_V8);
}

void InterpreterAssembly::HandleCloseiteratorImm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CLOSEITERATOR_IMM16_V8);
}

void InterpreterAssembly::HandleNewobjspreadImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(NEWOBJSPREAD_IMM16_V8_V8);
}

void InterpreterAssembly::HandleSetobjectwithprotoImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(SETOBJECTWITHPROTO_IMM16_V8_V8);
}

void InterpreterAssembly::HandleLdobjbyvalueImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(LDOBJBYVALUE_IMM16_V8_V8);
}

void InterpreterAssembly::HandleStobjbyvalueImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STOBJBYVALUE_IMM16_V8_V8);
}

void InterpreterAssembly::HandleStownbyvalueImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STOWNBYVALUE_IMM16_V8_V8);
}

void InterpreterAssembly::HandleLdsuperbyvalueImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(LDSUPERBYVALUE_IMM16_V8_V8);
}

void InterpreterAssembly::HandleStsuperbyvalueImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STSUPERBYVALUE_IMM16_V8_V8);
}

void InterpreterAssembly::HandleLdobjbyindexImm16V8Imm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(LDOBJBYINDEX_IMM16_V8_IMM32);
}

void InterpreterAssembly::HandleStobjbyindexImm16V8Imm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STOBJBYINDEX_IMM16_V8_IMM32);
}

void InterpreterAssembly::HandleStownbyindexImm16V8Imm32(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STOWNBYINDEX_IMM16_V8_IMM32);
}

void InterpreterAssembly::HandleNewobjrangeImm16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(NEWOBJRANGE_IMM16_IMM16_V8);
}

void InterpreterAssembly::HandleDefinefuncImm16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINEFUNC_IMM16_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleDefinencfuncImm16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINENCFUNC_IMM16_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleDefinegeneratorfuncImm16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINEGENERATORFUNC_IMM16_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleDefineasyncfuncImm16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINEASYNCFUNC_IMM16_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleDefinemethodImm16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINEMETHOD_IMM16_ID16_IMM16_V8);
}

void InterpreterAssembly::HandleCreatearraywithbufferImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CREATEARRAYWITHBUFFER_IMM16_IMM16);
}

void InterpreterAssembly::HandleCreateobjecthavingmethodImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CREATEOBJECTHAVINGMETHOD_IMM16_IMM16);
}

void InterpreterAssembly::HandleCreateobjectwithbufferImm16Imm16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(CREATEOBJECTWITHBUFFER_IMM16_IMM16);
}

void InterpreterAssembly::HandleDefineclasswithbufferImm16Id16Imm16Imm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINECLASSWITHBUFFER_IMM16_ID16_IMM16_IMM16_V8_V8);
}

void InterpreterAssembly::HandleTryldglobalbynameImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(TRYLDGLOBALBYNAME_IMM16_ID16);
}

void InterpreterAssembly::HandleTrystglobalbynameImm16Id16(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(TRYSTGLOBALBYNAME_IMM16_ID16);
}

void InterpreterAssembly::HandleLdobjbynameImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(LDOBJBYNAME_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleStobjbynameImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STOBJBYNAME_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleStownbynameImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STOWNBYNAME_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleLdsuperbynameImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(LDSUPERBYNAME_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleStsuperbynameImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STSUPERBYNAME_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleStownbyvaluewithnamesetImm16V8V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STOWNBYVALUEWITHNAMESET_IMM16_V8_V8);
}

void InterpreterAssembly::HandleStownbynamewithnamesetImm16Id16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8);
}

void InterpreterAssembly::HandleDefineasyncgeneratorfuncImm16Id16Imm16V8(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    DISPATCH(DEFINEASYNCGENERATORFUNC_IMM16_ID16_IMM16_V8);
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
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            thread->SetLastFp(frameHandler.GetFp());
            return;
        }
        auto method = frameHandler.GetMethod();
        pcOffset = FindCatchBlock(method, frameHandler.GetBytecodeOffset());
        if (pcOffset != panda_file::INVALID_OFFSET) {
            thread->SetCurrentFrame(frameHandler.GetSp());
            thread->SetLastFp(frameHandler.GetFp());
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            pc = method->GetBytecodeArray() + pcOffset;
            break;
        }
    }
    if (pcOffset == panda_file::INVALID_OFFSET) {
        return;
    }

    auto exception = thread->GetException();
    SET_ACC(exception);
    thread->ClearException();
    DISPATCH_OFFSET(0);
}

void InterpreterAssembly::HandleOverflow(
    JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
    JSTaggedValue acc, int16_t hotnessCounter)
{
    LOG_INTERPRETER(FATAL) << "opcode overflow";
}

uint32_t InterpreterAssembly::FindCatchBlock(Method *caller, uint32_t pc)
{
    auto *pandaFile = caller->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pandaFile, caller->GetMethodId());
    panda_file::CodeDataAccessor cda(*pandaFile, mda.GetCodeId().value());

    uint32_t pcOffset = panda_file::INVALID_OFFSET;
    cda.EnumerateTryBlocks([&pcOffset, pc](panda_file::CodeDataAccessor::TryBlock &try_block) {
        if ((try_block.GetStartPc() <= pc) && ((try_block.GetStartPc() + try_block.GetLength()) > pc)) {
            try_block.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catch_block) {
                pcOffset = catch_block.GetHandlerPc();
                return false;
            });
        }
        return pcOffset == panda_file::INVALID_OFFSET;
    });
    return pcOffset;
}

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

JSTaggedValue InterpreterAssembly::GetThisFunction(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    return JSTaggedValue(state->function);
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
    if (profileTypeInfo == JSTaggedValue::Undefined()) {
        auto method = function->GetCallTarget();
        auto res = SlowRuntimeStub::NotifyInlineCache(thread, function, method);
        return res;
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
