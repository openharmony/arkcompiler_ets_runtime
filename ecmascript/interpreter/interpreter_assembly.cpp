/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ArkCompiler::InterpreterAssembly::Execute", "");
#endif
    // When the  function is jit-compiled, the Method object is reinstalled.
    // In this case, the AotWithCall field may be updated.
    // This causes a Construct that is not a ClassConstructor to call jit code.
    ECMAObject *callTarget = reinterpret_cast<ECMAObject*>(info->GetFunctionValue().GetTaggedObject());
    Method *method = callTarget->GetCallTarget(thread);
    bool isCompiledCode = JSFunctionBase::IsCompiledCodeFromCallTarget(thread, info->GetFunctionValue());

    // check is or not debugger
    thread->CheckSwitchDebuggerBCStub();
    thread->CheckSwitchRBStub();
    thread->CheckSafepoint();
    uint32_t argc = info->GetArgsNumber();
    uintptr_t argv = reinterpret_cast<uintptr_t>(info->GetArgs());

    callTarget = reinterpret_cast<ECMAObject*>(info->GetFunctionValue().GetTaggedObject());
    method = callTarget->GetCallTarget(thread);
    if (isCompiledCode && thread->HasSwitchedToStwStub()) {
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
        JSTaggedValue env = func->GetLexicalEnv(thread);
        MethodEntry(thread, method, env);
    }
    if (thread->NeedReadBarrier()) {
        base::GCHelper::CopyCallTarget(thread, callTarget); // callTarget should be ToSpace Reference
        method = callTarget->GetCallTarget(thread);
    }
    // When C++ enters ASM, save the current globalenv and restore to glue after call
    SaveEnv envScope(thread);
    JSTaggedType acc;
    {
#ifdef USE_HWASAN
        HandleScopeDepthScope depthScope(thread->GetEcmaVM());
#endif
        auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_AsmInterpreterEntry);
        acc = reinterpret_cast<InterpreterEntry>(entry)(thread->GetGlueAddr(),
            callTarget, method, method->GetCallField(), argc, argv);
    }

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
    auto *debuggerMgr = thread->GetEcmaVM()->GetJsDebuggerManager();
    FrameHandler frameHandler(thread);
    // No frame before the first method is executed
    if (!frameHandler.HasFrame()) {
        debuggerMgr->GetNotificationManager()->MethodEntryEvent(thread, method, env);
        return;
    }

    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame()) {
            continue;
        }
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
    JSTaggedValue func = context->GetMethod(thread);
    Method *method = ECMAObject::Cast(func.GetTaggedObject())->GetCallTarget(thread);
    JSTaggedValue env = context->GetLexicalEnv(thread);
    if (thread->IsDebugMode() && !method->IsNativeWithCallField()) {
        MethodEntry(thread, method, env);
    }
    if (thread->NeedReadBarrier()) {
        // func should be ToSpace Reference
        base::GCHelper::CopyCallTarget(thread, func.GetTaggedObject());
        // context should be ToSpace Reference
        base::GCHelper::CopyGeneratorContext(thread, context.GetObject<GeneratorContext>());
    }
    auto acc = reinterpret_cast<GeneratorReEnterInterpEntry>(entry)(thread->GetGlueAddr(), context.GetTaggedType());
    return JSTaggedValue(acc);
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

JSTaggedValue InterpreterAssembly::GetNewTarget(JSThread *thread, JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget(thread);
    ASSERT(method->HaveNewTargetWithCallField());
    uint32_t numVregs = method->GetNumVregsWithCallField();
    bool haveFunc = method->HaveFuncWithCallField();
    return JSTaggedValue(sp[numVregs + haveFunc]);
}

JSTaggedValue InterpreterAssembly::GetConstantPool(JSThread *thread, JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget(thread);
    return method->GetConstantPool(thread);
}

JSTaggedValue InterpreterAssembly::GetUnsharedConstpool(JSThread *thread, JSTaggedType *sp)
{
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget(thread);
    return thread->GetEcmaVM()->FindOrCreateUnsharedConstpool(method->GetConstantPool(thread));
}

JSTaggedValue InterpreterAssembly::GetModule(JSThread *thread, JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    return JSFunction::Cast(state->function.GetTaggedObject())->GetModule(thread);
}

JSTaggedValue InterpreterAssembly::GetProfileTypeInfo(JSThread *thread, JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    JSFunction *function = JSFunction::Cast(state->function.GetTaggedObject());
    return function->GetProfileTypeInfo(thread);
}

JSTaggedType *InterpreterAssembly::GetAsmInterpreterFramePointer(AsmInterpretedFrame *state)
{
    return state->GetCurrentFramePointer();
}

uint32_t InterpreterAssembly::GetNumArgs(JSThread *thread, JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    AsmInterpretedFrame *state = reinterpret_cast<AsmInterpretedFrame *>(sp) - 1;
    Method *method = JSFunction::Cast(state->function.GetTaggedObject())->GetCallTarget(thread);
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
}  // namespace panda::ecmascript
