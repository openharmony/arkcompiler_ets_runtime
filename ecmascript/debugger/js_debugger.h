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
        const Global<FunctionRef> &condFuncRef, const std::string &url,
        int32_t line, int32_t column) : sourceFile_(sourceFile), ptMethod_(ptMethod),
        bcOffset_(bcOffset), condFuncRef_(condFuncRef), url_(url), line_(line), column_(column) {}
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

    const std::string &GetUrl() const
    {
        return url_;
    }

    int32_t GetLine() const
    {
        return line_;
    }

    int32_t GetColumn() const
    {
        return column_;
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
        breakpoint << "url:" << "\""<< url_ << "\""<< ", ";
        breakpoint << "line:" << line_ << ", ";
        breakpoint << "column:" << column_;
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

    // Used by new Breakpoint logic
    std::string url_;
    int32_t line_;
    int32_t column_;
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
    bool SetUrlNotMatchedBreakpoint(const JSPtLocation &location);
    std::vector<bool> SetBreakpointUsingList(std::vector<JSPtLocation> &list);
    bool RemoveUrlNotMatchedBreakpoint(const JSPtLocation &location);
    bool RemoveAllBreakpointsByUrl(const std::string &url, bool skipGlobal);
private:
    std::unique_ptr<PtMethod> FindMethod(const JSPtLocation &location) const;
    std::optional<JSBreakpoint> FindSmartBreakpoint(JSHandle<Method> method, uint32_t bcOffset) const;
    bool RemoveSmartBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, uint32_t bcOffset);
    void HandleExceptionThrowEvent(const JSThread *thread, JSHandle<Method> method, uint32_t bcOffset);
    bool HandleStep(JSHandle<Method> method, uint32_t bcOffset);
    bool HandleNativeOut();
    bool HandleBreakpoint(JSHandle<Method> method, uint32_t bcOffset);
    void DumpBreakpoints();
    bool IsBreakpointCondSatisfied(std::optional<JSBreakpoint> breakpoint) const;
    bool PushBreakpointToLocal(const JSBreakpoint &breakpoint, const JSPtLocation &location);
    // All methods which manipulates the globalBpList_ should be called with write lock acquired
    // All methods thatsearches/pulls breakpoints from the globalBpList_ should be used with read lock acquired
    std::optional<JSBreakpoint> FindLocalBreakpoint(JSHandle<Method> method, uint32_t bcOffset);
    bool RemoveLocalBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, uint32_t bcOffset);
    bool SetBreakpointWithMatchedUrl(const JSPtLocation &location, Global<FunctionRef> condFuncRef);
    bool SetBreakpointWithoutMatchedUrl(const JSPtLocation &location);
    std::vector<bool> SetBreakpointByList(std::vector<JSPtLocation> &list);
    void PushBreakpointToGlobal(const JSBreakpoint &breakpoint, const JSPtLocation &location);
    bool PullBreakpointFromGlobal(const JSBreakpoint &breakpoint);
    void DumpGlobalBreakpoints();
    std::optional<JSBreakpoint> SearchBreakpointInGlobalList(const JSPtLocation &location, PtMethod *ptMethod);
    std::optional<JSBreakpoint> SearchBreakpointInGlobalList(JSHandle<Method> method, uint32_t bcOffset);
    std::vector<JSBreakpoint> SearchNoMatchBreakpointInLocalList(const JSPtLocation &location);
    std::vector<JSBreakpoint> SearchNoMatchBreakpointInGlobalList(const JSPtLocation &location);
    bool RemoveGlobalBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, const JSPtLocation &location);
    bool RemoveGlobalBreakpoint(const JSPtLocation &location);
    bool RemoveGlobalBreakpoint(const std::string &url);

    const EcmaVM *ecmaVm_;
    PtHooks *hooks_ {nullptr};
    NotificationManager *notificationMgr_ {nullptr};
    bool singleStepOnDebuggerStmt_ {false};

    static std::shared_mutex listMutex_;
    // Map<JSPandafile, Map<url, Set<JSBreakpoint>>>
    static CUnorderedMap<CString, CUnorderedMap<std::string,
        CUnorderedSet<JSBreakpoint, HashJSBreakpoint>>> globalBpList_;
    CUnorderedMap<CString, CUnorderedMap<std::string,
        CUnorderedSet<JSBreakpoint, HashJSBreakpoint>>> breakpoints_ {};
    CUnorderedSet<JSBreakpoint, HashJSBreakpoint> smartBreakpoints_ {};
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_DEBUGGER_JS_DEBUGGER_H