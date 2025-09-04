/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEBUGGER_NOTIFICATION_MANAGER_H
#define ECMASCRIPT_DEBUGGER_NOTIFICATION_MANAGER_H

#include <string_view>

#include "ecmascript/dfx/stackinfo/async_stack_trace.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::tooling {
class RuntimeListener {
public:
    RuntimeListener() = default;
    virtual ~RuntimeListener() = default;
    DEFAULT_COPY_SEMANTIC(RuntimeListener);
    DEFAULT_MOVE_SEMANTIC(RuntimeListener);

    virtual void LoadModule(std::string_view name, std::string_view) = 0;

    virtual void BytecodePcChanged(JSThread *thread, JSHandle<Method> method,
                                   uint32_t bcOffset) = 0;
    
    virtual bool HandleDebuggerStmt(JSHandle<Method> method, uint32_t bcOffset) = 0;
    virtual void VmStart() = 0;
    virtual void VmDeath() = 0;
    virtual void NativeCalling(const void *nativeAddress) = 0;
    virtual void NativeReturn(const void *nativeAddress) = 0;
    virtual void MethodEntry(JSHandle<Method> method, JSHandle<JSTaggedValue> envHandle) = 0;
    virtual void MethodExit(JSHandle<Method> method) = 0;
    virtual void GenerateAsyncFrames(std::shared_ptr<AsyncStack> asyncStack, bool skipTopFrame) = 0;
    virtual void SetDebuggerAccessor(const JSHandle<GlobalEnv> &globalEnv) = 0;
};

class NotificationManager {
public:
    NotificationManager() = default;
    ~NotificationManager() = default;
    NO_COPY_SEMANTIC(NotificationManager);
    NO_MOVE_SEMANTIC(NotificationManager);

    void AddListener(RuntimeListener *listener)
    {
        if (listener != nullptr) {
            listeners_.emplace_back(listener);
        }
    }
    void RemoveListener(RuntimeListener *listener)
    {
        for (auto it = listeners_.begin(); it != listeners_.end(); ++it) {
            if (*it == listener) {
                listeners_.erase(it);
                return;
            }
        }
    }

    void GenerateAsyncFramesEvent(std::shared_ptr<AsyncStack> asyncStack, bool skipTopFrame) const
    {
        for (auto it: listeners_) {
            it->GenerateAsyncFrames(asyncStack, skipTopFrame);
        }
    }

    void LoadModuleEvent(std::string_view name, std::string_view entryPoint) const
    {
        for (auto it: listeners_) {
            it->LoadModule(name, entryPoint);
        }
    }

    void BytecodePcChangedEvent(JSThread *thread, Method *method, uint32_t bcOffset) const
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        JSHandle<Method> methodHandle(thread, method);
        for (auto it: listeners_) {
            it->BytecodePcChanged(thread, methodHandle, bcOffset);
        }
    }

    void DebuggerStmtEvent(JSThread *thread, Method *method, uint32_t bcOffset) const
    {
        JSHandle<Method> methodHandle(thread, method);
        for (auto it: listeners_) {
            it->HandleDebuggerStmt(methodHandle, bcOffset);
        }
    }

    void NativeCallingEvent(const void *nativeAddress) const
    {
        for (auto it: listeners_) {
            it->NativeCalling(nativeAddress);
        }
    }

    void NativeReturnEvent(const void *nativeAddress) const
    {
        for (auto it: listeners_) {
            it->NativeReturn(nativeAddress);
        }
    }

    void VmStartEvent() const
    {
        for (auto it: listeners_) {
            it->VmStart();
        }
    }
    void VmDeathEvent() const
    {
        for (auto it: listeners_) {
            it->VmDeath();
        }
    }

    void MethodEntryEvent(JSThread *thread, Method *method, JSTaggedValue env) const
    {
        JSHandle<Method> methodHandle(thread, method);
        JSHandle<JSTaggedValue> envHandle(thread, env);
        for (auto it: listeners_) {
            it->MethodEntry(methodHandle, envHandle);
        }
    }
    void MethodExitEvent(JSThread *thread, Method *method) const
    {
        JSHandle<Method> methodHandle(thread, method);
        for (auto it: listeners_) {
            it->MethodExit(methodHandle);
        }
    }

    void SetDebuggerAccessorEvent(const JSHandle<GlobalEnv> &globalEnv) const
    {
        for (auto it: listeners_) {
            it->SetDebuggerAccessor(globalEnv);
        }
    }
private:
    std::vector<RuntimeListener*> listeners_;
};
}  // panda::ecmascript::tooling

#endif  // ECMASCRIPT_DEBUGGER_NOTIFICATION_MANAGER_H