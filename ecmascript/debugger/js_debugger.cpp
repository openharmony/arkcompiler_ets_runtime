/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/debugger/js_debugger.h"

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::base::BuiltinsBase;

bool JSDebugger::SetBreakpoint(const JSPtLocation &location, Local<FunctionRef> condFuncRef)
{
    std::unique_ptr<PtMethod> ptMethod = FindMethod(location);
    if (ptMethod == nullptr) {
        LOG_DEBUGGER(ERROR) << "SetBreakpoint: Cannot find MethodLiteral";
        return false;
    }

    if (location.GetBytecodeOffset() >= ptMethod->GetCodeSize()) {
        LOG_DEBUGGER(ERROR) << "SetBreakpoint: Invalid breakpoint location";
        return false;
    }

    auto [_, success] = breakpoints_.emplace(location.GetSourceFile(), ptMethod.release(),
        location.GetBytecodeOffset(), Global<FunctionRef>(ecmaVm_, condFuncRef));
    if (!success) {
        // also return true
        LOG_DEBUGGER(WARN) << "SetBreakpoint: Breakpoint already exists";
    }

    DumpBreakpoints();
    return true;
}

bool JSDebugger::RemoveBreakpoint(const JSPtLocation &location)
{
    std::unique_ptr<PtMethod> ptMethod = FindMethod(location);
    if (ptMethod == nullptr) {
        LOG_DEBUGGER(ERROR) << "RemoveBreakpoint: Cannot find MethodLiteral";
        return false;
    }

    if (!RemoveBreakpoint(ptMethod, location.GetBytecodeOffset())) {
        LOG_DEBUGGER(ERROR) << "RemoveBreakpoint: Breakpoint not found";
        return false;
    }

    DumpBreakpoints();
    return true;
}

void JSDebugger::RemoveAllBreakpoints()
{
    breakpoints_.clear();
}

void JSDebugger::BytecodePcChanged(JSThread *thread, JSHandle<Method> method, uint32_t bcOffset)
{
    ASSERT(bcOffset < method->GetCodeSize() && "code size of current Method less then bcOffset");
    HandleExceptionThrowEvent(thread, method, bcOffset);

    // Step event is reported before breakpoint, according to the spec.
    if (!HandleStep(method, bcOffset)) {
        HandleBreakpoint(method, bcOffset);
    }
}

bool JSDebugger::HandleBreakpoint(JSHandle<Method> method, uint32_t bcOffset)
{
    auto breakpoint = FindBreakpoint(method, bcOffset);
    if (hooks_ == nullptr || !breakpoint.has_value()) {
        return false;
    }

    JSThread *thread = ecmaVm_->GetJSThread();
    auto condFuncRef = breakpoint.value().GetConditionFunction();
    if (condFuncRef->IsFunction()) {
        LOG_DEBUGGER(INFO) << "HandleBreakpoint: begin evaluate condition";
        auto handlerPtr = std::make_shared<FrameHandler>(ecmaVm_->GetJSThread());
        auto res = DebuggerApi::EvaluateViaFuncCall(const_cast<EcmaVM *>(ecmaVm_),
            condFuncRef.ToLocal(ecmaVm_), handlerPtr);
        if (thread->HasPendingException()) {
            LOG_DEBUGGER(ERROR) << "HandleBreakpoint: has pending exception";
            thread->ClearException();
            return false;
        }
        bool isMeet = res->ToBoolean(ecmaVm_)->Value();
        if (!isMeet) {
            LOG_DEBUGGER(ERROR) << "HandleBreakpoint: condition not meet";
            return false;
        }
    }

    JSPtLocation location {method->GetJSPandaFile(), method->GetMethodId(), bcOffset,
        breakpoint.value().GetSourceFile()};

    hooks_->Breakpoint(location);
    return true;
}

bool JSDebugger::HandleDebuggerStmt(JSHandle<Method> method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !ecmaVm_->GetJsDebuggerManager()->IsDebugMode()) {
        return false;
    }

    JSPtLocation location {method->GetJSPandaFile(), method->GetMethodId(), bcOffset};
    hooks_->DebuggerStmt(location);

    return true;
}

void JSDebugger::HandleExceptionThrowEvent(const JSThread *thread, JSHandle<Method> method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !thread->HasPendingException()) {
        return;
    }

    JSPtLocation throwLocation {method->GetJSPandaFile(), method->GetMethodId(), bcOffset};

    hooks_->Exception(throwLocation);
}

bool JSDebugger::HandleStep(JSHandle<Method> method, uint32_t bcOffset)
{
    if (hooks_ == nullptr) {
        return false;
    }

    JSPtLocation location {method->GetJSPandaFile(), method->GetMethodId(), bcOffset};

    return hooks_->SingleStep(location);
}

std::optional<JSBreakpoint> JSDebugger::FindBreakpoint(JSHandle<Method> method, uint32_t bcOffset) const
{
    for (const auto &bp : breakpoints_) {
        if ((bp.GetBytecodeOffset() == bcOffset) &&
            (bp.GetPtMethod()->GetJSPandaFile() == method->GetJSPandaFile()) &&
            (bp.GetPtMethod()->GetMethodId() == method->GetMethodId())) {
            return bp;
        }
    }
    return {};
}

bool JSDebugger::RemoveBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, uint32_t bcOffset)
{
    for (auto it = breakpoints_.begin(); it != breakpoints_.end(); ++it) {
        const auto &bp = *it;
        if ((bp.GetBytecodeOffset() == bcOffset) &&
            (bp.GetPtMethod()->GetJSPandaFile() == ptMethod->GetJSPandaFile()) &&
            (bp.GetPtMethod()->GetMethodId() == ptMethod->GetMethodId())) {
            it = breakpoints_.erase(it);
            return true;
        }
    }

    return false;
}

std::unique_ptr<PtMethod> JSDebugger::FindMethod(const JSPtLocation &location) const
{
    std::unique_ptr<PtMethod> ptMethod {nullptr};
    ::panda::ecmascript::JSPandaFileManager::GetInstance()->EnumerateJSPandaFiles([&ptMethod, location](
        const JSPandaFile *jsPandaFile) {
        if (jsPandaFile->GetJSPandaFileDesc() == location.GetJsPandaFile()->GetJSPandaFileDesc()) {
            MethodLiteral *methodsData = jsPandaFile->GetMethodLiterals();
            uint32_t numberMethods = jsPandaFile->GetNumMethods();
            for (uint32_t i = 0; i < numberMethods; ++i) {
                if (methodsData[i].GetMethodId() == location.GetMethodId()) {
                    MethodLiteral *methodLiteral = methodsData + i;
                    ptMethod = std::make_unique<PtMethod>(jsPandaFile,
                        methodLiteral->GetMethodId(), methodLiteral->IsNativeWithCallField());
                    return false;
                }
            }
        }
        return true;
    });
    return ptMethod;
}

void JSDebugger::DumpBreakpoints()
{
    LOG_DEBUGGER(INFO) << "dump breakpoints with size " << breakpoints_.size();
    for (const auto &bp : breakpoints_) {
        LOG_DEBUGGER(DEBUG) << bp.ToString();
    }
}

void JSDebugger::MethodEntry(JSHandle<Method> method, JSHandle<JSTaggedValue> envHandle)
{
    if (hooks_ == nullptr || !ecmaVm_->GetJsDebuggerManager()->IsDebugMode()) {
        return;
    }
    FrameHandler frameHandler(ecmaVm_->GetJSThread());
    if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
        return;
    }
    auto *debuggerMgr = ecmaVm_->GetJsDebuggerManager();
    debuggerMgr->MethodEntry(method, envHandle);
}

void JSDebugger::MethodExit([[maybe_unused]] JSHandle<Method> method)
{
    if (hooks_ == nullptr || !ecmaVm_->GetJsDebuggerManager()->IsDebugMode()) {
        return;
    }
    FrameHandler frameHandler(ecmaVm_->GetJSThread());
    if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
        return;
    }
    auto *debuggerMgr = ecmaVm_->GetJsDebuggerManager();
    debuggerMgr->MethodExit(method);
}
}  // namespace panda::tooling::ecmascript
