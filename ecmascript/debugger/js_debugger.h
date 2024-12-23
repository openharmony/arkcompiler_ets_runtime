/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEBUGGER_JS_DEBUGGER_H
#define ECMASCRIPT_DEBUGGER_JS_DEBUGGER_H

#include "ecmascript/debugger/debugger_api.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/debugger/js_pt_method.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript::tooling {
class JSBreakpoint {
public:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    JSBreakpoint(const std::string &sourceFile, PtMethod *ptMethod, uint32_t bcOffset,
        const Global<FunctionRef> &condFuncRef) : sourceFile_(sourceFile), ptMethod_(ptMethod),
        bcOffset_(bcOffset), condFuncRef_(condFuncRef) {}
    ~JSBreakpoint() = default;

    const std::string &GetSourceFile() const
    {
        return sourceFile_;
    }

    PtMethod *GetPtMethod() const
    {
        return ptMethod_;
    }

    uint32_t GetBytecodeOffset() const
    {
        return bcOffset_;
    }

    bool operator==(const JSBreakpoint &bpoint) const
    {
        return bcOffset_ == bpoint.GetBytecodeOffset() &&
            ptMethod_->GetMethodId() == bpoint.GetPtMethod()->GetMethodId() &&
            sourceFile_ == bpoint.GetSourceFile() &&
            ptMethod_->GetJSPandaFile() == bpoint.GetPtMethod()->GetJSPandaFile();
    }

    std::string ToString() const
    {
        std::stringstream breakpoint;
        breakpoint << "[";
        breakpoint << "methodId:" << ptMethod_->GetMethodId()  << ", ";
        breakpoint << "bytecodeOffset:" << bcOffset_ << ", ";
        breakpoint << "sourceFile:" << "\""<< sourceFile_ << "\""<< ", ";
        breakpoint << "jsPandaFile:" << "\"" << ptMethod_->GetJSPandaFile()->GetJSPandaFileDesc() << "\"";
        breakpoint << "]";
        return breakpoint.str();
    }

    const Global<FunctionRef> &GetConditionFunction()
    {
        return condFuncRef_;
    }

    DEFAULT_COPY_SEMANTIC(JSBreakpoint);
    DEFAULT_MOVE_SEMANTIC(JSBreakpoint);

private:
    std::string sourceFile_;
    PtMethod *ptMethod_ {nullptr};
    uint32_t bcOffset_;
    Global<FunctionRef> condFuncRef_;
};

class HashJSBreakpoint {
public:
    size_t operator()(const JSBreakpoint &bpoint) const
    {
        return (std::hash<std::string>()(bpoint.GetSourceFile())) ^
            (std::hash<uint32_t>()(bpoint.GetPtMethod()->GetMethodId().GetOffset())) ^
            (std::hash<uint32_t>()(bpoint.GetBytecodeOffset()));
    }
};

class JSDebugger : public JSDebugInterface, RuntimeListener {
public:
    explicit JSDebugger(const EcmaVM *vm) : ecmaVm_(vm)
    {
        notificationMgr_ = ecmaVm_->GetJsDebuggerManager()->GetNotificationManager();
        notificationMgr_->AddListener(this);
    }
    ~JSDebugger() override
    {
        notificationMgr_->RemoveListener(this);
    }

    void RegisterHooks(PtHooks *hooks) override
    {
        hooks_ = hooks;
        // send vm start event after add hooks
        notificationMgr_->VmStartEvent();
    }
    void UnregisterHooks() override
    {
        // send vm death event before delete hooks
        notificationMgr_->VmDeathEvent();
        hooks_ = nullptr;
    }
    bool HandleDebuggerStmt(JSHandle<Method> method, uint32_t bcOffset) override;
    bool SetBreakpoint(const JSPtLocation &location, Local<FunctionRef> condFuncRef) override;
    bool SetSmartBreakpoint(const JSPtLocation &location);
    bool RemoveBreakpoint(const JSPtLocation &location) override;
    void RemoveAllBreakpoints() override;
    bool RemoveBreakpointsByUrl(const std::string &url) override;
    void BytecodePcChanged(JSThread *thread, JSHandle<Method> method, uint32_t bcOffset) override;
    void LoadModule(std::string_view filename, std::string_view entryPoint) override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->LoadModule(filename, entryPoint);
    }
    void VmStart() override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->VmStart();
    }
    void VmDeath() override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->VmDeath();
    }
    void NativeCalling(const void *nativeAddress) override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->NativeCalling(nativeAddress);
    }
    void NativeReturn(const void *nativeAddress) override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->NativeReturn(nativeAddress);
    }
    void MethodEntry(JSHandle<Method> method, JSHandle<JSTaggedValue> envHandle) override;
    void MethodExit(JSHandle<Method> method) override;
    // used by debugger statement
    bool GetSingleStepStatus() const
    {
        return singleStepOnDebuggerStmt_;
    }
    void SetSingleStepStatus(bool status)
    {
        singleStepOnDebuggerStmt_ = status;
    }
    // Used by Launch Accelerate mode
    void DisableFirstTimeFlag()
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->DisableFirstTimeFlag();
    }
private:
    std::unique_ptr<PtMethod> FindMethod(const JSPtLocation &location) const;
    std::optional<JSBreakpoint> FindBreakpoint(JSHandle<Method> method, uint32_t bcOffset) const;
    std::optional<JSBreakpoint> FindSmartBreakpoint(JSHandle<Method> method, uint32_t bcOffset) const;
    bool RemoveBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, uint32_t bcOffset);
    bool RemoveSmartBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, uint32_t bcOffset);
    void HandleExceptionThrowEvent(const JSThread *thread, JSHandle<Method> method, uint32_t bcOffset);
    bool HandleStep(JSHandle<Method> method, uint32_t bcOffset);
    bool HandleNativeOut();
    bool HandleBreakpoint(JSHandle<Method> method, uint32_t bcOffset);
    void DumpBreakpoints();
    bool IsBreakpointCondSatisfied(std::optional<JSBreakpoint> breakpoint) const;

    const EcmaVM *ecmaVm_;
    PtHooks *hooks_ {nullptr};
    NotificationManager *notificationMgr_ {nullptr};
    bool singleStepOnDebuggerStmt_ {false};

    CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpoints_ {};
    CUnorderedSet<JSBreakpoint, HashJSBreakpoint> smartBreakpoints_ {};

    friend class JsDebuggerFriendTest;
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_DEBUGGER_JS_DEBUGGER_H
