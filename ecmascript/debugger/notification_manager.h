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

#ifndef ECMASCRIPT_DEBUGGER_NOTIFICATION_MANAGER_H
#define ECMASCRIPT_DEBUGGER_NOTIFICATION_MANAGER_H

#include <string_view>

#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"

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
    virtual void MethodEntry(JSHandle<Method> method, JSHandle<JSTaggedValue> envHandle) = 0;
    virtual void MethodExit(JSHandle<Method> method) = 0;
};

class NotificationManager {
public:
    NotificationManager() = default;
    ~NotificationManager() = default;
    NO_COPY_SEMANTIC(NotificationManager);
    NO_MOVE_SEMANTIC(NotificationManager);

    void AddListener(RuntimeListener *listener)
    {
        listener_ = listener;
    }
    void RemoveListener()
    {
        listener_ = nullptr;
    }

    void LoadModuleEvent(std::string_view name, std::string_view entryPoint) const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            listener_->LoadModule(name, entryPoint);
        }
    }

    void BytecodePcChangedEvent(JSThread *thread, Method *method, uint32_t bcOffset) const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            JSHandle<Method> methodHandle(thread, method);
            listener_->BytecodePcChanged(thread, methodHandle, bcOffset);
        }
    }

    void DebuggerStmtEvent(JSThread *thread, Method *method, uint32_t bcOffset) const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            JSHandle<Method> methodHandle(thread, method);
            listener_->HandleDebuggerStmt(methodHandle, bcOffset);
        }
    }

    void NativeCallingEvent(const void *nativeAddress) const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            listener_->NativeCalling(nativeAddress);
        }
    }

    void VmStartEvent() const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            listener_->VmStart();
        }
    }
    void VmDeathEvent() const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            listener_->VmDeath();
        }
    }

    void MethodEntryEvent(JSThread *thread, Method *method, JSTaggedValue env) const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            JSHandle<Method> methodHandle(thread, method);
            JSHandle<JSTaggedValue> envHandle(thread, env);
            listener_->MethodEntry(methodHandle, envHandle);
        }
    }
    void MethodExitEvent(JSThread *thread, Method *method) const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            JSHandle<Method> methodHandle(thread, method);
            listener_->MethodExit(methodHandle);
        }
    }
private:
    RuntimeListener *listener_ {nullptr};
};
}  // panda::ecmascript::tooling

#endif  // ECMASCRIPT_DEBUGGER_NOTIFICATION_MANAGER_H