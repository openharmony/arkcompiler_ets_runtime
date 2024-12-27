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

bool JSDebugger::SetSmartBreakpoint(const JSPtLocation &location)
{
    std::unique_ptr<PtMethod> ptMethod = FindMethod(location);
    if (ptMethod == nullptr) {
        LOG_DEBUGGER(ERROR) << "SetSmartBreakpoint: Cannot find MethodLiteral";
        return false;
    }

    if (location.GetBytecodeOffset() >= ptMethod->GetCodeSize()) {
        LOG_DEBUGGER(ERROR) << "SetSmartBreakpoint: Invalid breakpoint location";
        return false;
    }

    auto [_, success] = smartBreakpoints_.emplace(location.GetSourceFile(), ptMethod.release(),
        location.GetBytecodeOffset(), Global<FunctionRef>(ecmaVm_, FunctionRef::Undefined(ecmaVm_)));
    if (!success) {
        // also return true
        LOG_DEBUGGER(WARN) << "SetSmartBreakpoint: Breakpoint already exists";
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

bool JSDebugger::RemoveBreakpointsByUrl(const std::string &url)
{
    for (auto it = breakpoints_.begin(); it != breakpoints_.end();) {
        const auto &bp = *it;
        if (bp.GetSourceFile() == url) {
            it = breakpoints_.erase(it);
        } else {
            it++;
        }
    }

    DumpBreakpoints();
    return true;
}

void JSDebugger::BytecodePcChanged(JSThread *thread, JSHandle<Method> method, uint32_t bcOffset)
{
    ASSERT(bcOffset < method->GetCodeSize() && "code size of current Method less then bcOffset");
    HandleExceptionThrowEvent(thread, method, bcOffset);
    // clear singlestep flag
    singleStepOnDebuggerStmt_ = false;
    if (ecmaVm_->GetJsDebuggerManager()->IsMixedStackEnabled()) {
        if (!HandleBreakpoint(method, bcOffset)) {
            HandleNativeOut();
            HandleStep(method, bcOffset);
        }
    } else  {
        if (!HandleStep(method, bcOffset)) {
            HandleBreakpoint(method, bcOffset);
        }
    }
}

bool JSDebugger::HandleNativeOut()
{
    if (hooks_ == nullptr) {
        return false;
    }

    return hooks_->NativeOut();
}

bool JSDebugger::HandleBreakpoint(JSHandle<Method> method, uint32_t bcOffset)
{
    if (hooks_ == nullptr) {
        return false;
    }

    auto smartBreakpoint = FindSmartBreakpoint(method, bcOffset);
    if (smartBreakpoint.has_value()) {
        JSPtLocation smartLocation {method->GetJSPandaFile(), method->GetMethodId(), bcOffset,
            smartBreakpoint.value().GetSourceFile()};
        std::unique_ptr<PtMethod> ptMethod = FindMethod(smartLocation);
        RemoveSmartBreakpoint(ptMethod, bcOffset);
        hooks_->Breakpoint(smartLocation);
        return true;
    }

    auto breakpoint = FindBreakpoint(method, bcOffset);
    if (!breakpoint.has_value() || !IsBreakpointCondSatisfied(breakpoint)) {
        return false;
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
    // if debugger stmt is met by single stepping, disable debugger
    // stmt to prevent pausing on this line twice
    if (singleStepOnDebuggerStmt_) {
        return false;
    }
    auto breakpointAtDebugger = FindBreakpoint(method, bcOffset);
    // if a breakpoint is set on the same line as debugger stmt,
    // the debugger stmt is ineffective
    if (breakpointAtDebugger.has_value()) {
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

std::optional<JSBreakpoint> JSDebugger::FindSmartBreakpoint(JSHandle<Method> method, uint32_t bcOffset) const
{
    for (const auto &bp : smartBreakpoints_) {
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

bool JSDebugger::RemoveSmartBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, uint32_t bcOffset)
{
    for (auto it = smartBreakpoints_.begin(); it != smartBreakpoints_.end(); ++it) {
        const auto &bp = *it;
        if ((bp.GetBytecodeOffset() == bcOffset) &&
            (bp.GetPtMethod()->GetJSPandaFile() == ptMethod->GetJSPandaFile()) &&
            (bp.GetPtMethod()->GetMethodId() == ptMethod->GetMethodId())) {
            it = smartBreakpoints_.erase(it);
            return true;
        }
    }

    return false;
}

std::unique_ptr<PtMethod> JSDebugger::FindMethod(const JSPtLocation &location) const
{
    std::unique_ptr<PtMethod> ptMethod {nullptr};
    ::panda::ecmascript::JSPandaFileManager::GetInstance()->EnumerateJSPandaFiles([&ptMethod, location](
        const std::shared_ptr<JSPandaFile> &file) {
        if (file->GetJSPandaFileDesc() == location.GetJsPandaFile()->GetJSPandaFileDesc()) {
            MethodLiteral *methodsData = file->GetMethodLiterals();
            uint32_t numberMethods = file->GetNumMethods();
            for (uint32_t i = 0; i < numberMethods; ++i) {
                if (methodsData[i].GetMethodId() == location.GetMethodId()) {
                    MethodLiteral *methodLiteral = methodsData + i;
                    ptMethod = std::make_unique<PtMethod>(file.get(),
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

bool JSDebugger::IsBreakpointCondSatisfied(std::optional<JSBreakpoint> breakpoint) const
{
    if (!breakpoint.has_value()) {
        return false;
    }
    JSThread *thread = ecmaVm_->GetJSThread();
    auto condFuncRef = breakpoint.value().GetConditionFunction();
    if (condFuncRef->IsFunction(ecmaVm_)) {
        LOG_DEBUGGER(INFO) << "BreakpointCondition: evaluating condition";
        auto handlerPtr = std::make_shared<FrameHandler>(ecmaVm_->GetJSThread());
        auto evalResult = DebuggerApi::EvaluateViaFuncCall(const_cast<EcmaVM *>(ecmaVm_),
            condFuncRef.ToLocal(ecmaVm_), handlerPtr);
        if (thread->HasPendingException()) {
            LOG_DEBUGGER(ERROR) << "BreakpointCondition: has pending exception";
            thread->ClearException();
            return false;
        }
        bool satisfied = evalResult->ToBoolean(ecmaVm_)->Value();
        if (!satisfied) {
            LOG_DEBUGGER(INFO) << "BreakpointCondition: condition not meet";
            return false;
        }
    }
    return true;
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

    // scriptParsed for sendable object
    if (method->IsSendableMethod()) {
        hooks_->SendableMethodEntry(method);
    }
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
