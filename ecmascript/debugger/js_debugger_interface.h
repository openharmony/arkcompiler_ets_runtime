/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_DEBUGGER_JS_DEBUG_INTERFACE_H
#define ECMASCRIPT_DEBUGGER_JS_DEBUG_INTERFACE_H

#include <string_view>

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/debugger/js_pt_location.h"

namespace panda::ecmascript {
class Method;

namespace tooling {
struct JSPtStepRange {
    uint32_t startBcOffset {0};
    uint32_t endBcOffset {0};
};

enum PauseReason {
    AMBIGUOUS,
    ASSERT,
    DEBUGCOMMAND,
    DOM,
    EVENTLISTENER,
    EXCEPTION,
    INSTRUMENTATION,
    OOM,
    OTHER,
    PROMISEREJECTION,
    XHR,
    BREAK_ON_START,
    STEP, // single step
    DEBUGGERSTMT, // debugger stmt
    NATIVE_OUT
};

class PtHooks {
public:
    PtHooks() = default;

    /**
     * \brief called by the ecmavm when the next statement being executed is debugger statement.
     * Thread where debugger statement hits is paused until continue or step event being received
     * @param thread Identifier of the thread where debugger statement hits. Now the callback is called
     * in the same thread
     * @param location debugger statement location
     */
    virtual void DebuggerStmt(const JSPtLocation &location) = 0;

    /**
     * \brief called by the ecmavm when breakpoint hits. Thread where breakpoint hits is stopped until
     * continue or step event will be received
     * @param thread Identifier of the thread where breakpoint hits. Now the callback is called in the same
     * thread
     * @param location Breakpoint location
     */
    virtual void Breakpoint(const JSPtLocation &location) = 0;

    /**
     * \brief called by the ecmavm when panda file is loaded
     * @param pandaFileName Path to panda file that is loaded
     */
    virtual void LoadModule(std::string_view pandaFileName, std::string_view entryPoint) = 0;

    /**
     * \brief called by the ecmavm when virtual machine start initialization
     */
    virtual void VmStart() = 0;

    /**
     * \brief called by the ecmavm when virtual machine death
     */
    virtual void VmDeath() = 0;

    virtual void Exception(const JSPtLocation &location) = 0;

    virtual bool SingleStep(const JSPtLocation &location) = 0;

    virtual void NativeCalling(const void *nativeAddress) = 0;

    virtual void NativeReturn(const void *nativeAddress) = 0;

    virtual bool NativeOut() = 0;

    virtual void SendableMethodEntry(JSHandle<Method> method) = 0;

    virtual void DisableFirstTimeFlag() = 0;

    virtual ~PtHooks() = default;

    NO_COPY_SEMANTIC(PtHooks);
    NO_MOVE_SEMANTIC(PtHooks);
};

class JSDebugInterface {
public:
    JSDebugInterface() = default;

    /**
     * \brief handle debugger statement event
     * @param method Current method
     * @param bcOffset Current bytecode offset
     */
    virtual bool HandleDebuggerStmt(JSHandle<Method> method, uint32_t bcOffset) = 0;

    /**
     * \brief Register debug hooks in the ecmavm
     * @param hooks Pointer to object that implements PtHooks interface
     */
    virtual void RegisterHooks(PtHooks *hooks) = 0;

    /**
     * \brief Unregister debug hooks in the ecmavm
     */
    virtual void UnregisterHooks() = 0;

    /**
     * \brief Set breakpoint to \param location with an optional \param condition
     * @param location Breakpoint location
     * @param condition Optional condition
     * @return Error if any errors occur
     */
    virtual bool SetBreakpoint(const JSPtLocation &location, Local<FunctionRef> condFuncRef) = 0;

    /**
     * \brief Remove breakpoint from \param location
     * @param location Breakpoint location
     * @return Error if any errors occur
     */
    virtual bool RemoveBreakpoint(const JSPtLocation &location) = 0;

    /**
     * \brief Remove breakpoints specified by url
     * @param url file url
     * @return Error if any errors occur
     */
    virtual bool RemoveBreakpointsByUrl(const std::string &url) = 0;

    /**
     * \brief Remove all breakpoints
     */
    virtual void RemoveAllBreakpoints() = 0;

    virtual ~JSDebugInterface() = default;

    NO_COPY_SEMANTIC(JSDebugInterface);
    NO_MOVE_SEMANTIC(JSDebugInterface);
};
}  // namespace tooling
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_DEBUGGER_JS_DEBUG_INTERFACE_H
