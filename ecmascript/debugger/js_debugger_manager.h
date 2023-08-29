/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEBUGGER_JS_DEBUGGER_MANAGER_H
#define ECMASCRIPT_DEBUGGER_JS_DEBUGGER_MANAGER_H

#include "ecmascript/debugger/hot_reload_manager.h"
#include "ecmascript/debugger/notification_manager.h"
#include "ecmascript/debugger/dropframe_manager.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/global_handle_collection.h"

#include "libpandabase/os/library_loader.h"

namespace panda::ecmascript::tooling {
class ProtocolHandler;
class JsDebuggerManager {
public:
    using LibraryHandle = os::library_loader::LibraryHandle;
    using ObjectUpdaterFunc =
        std::function<void(const FrameHandler *, std::string_view, Local<JSValueRef>)>;
    using SingleStepperFunc = std::function<void()>;

    explicit JsDebuggerManager(const EcmaVM *vm) : hotReloadManager_(vm)
    {
        jsThread_ = vm->GetJSThread();
    }
    ~JsDebuggerManager() = default;

    NO_COPY_SEMANTIC(JsDebuggerManager);
    NO_MOVE_SEMANTIC(JsDebuggerManager);

    NotificationManager *GetNotificationManager() const
    {
        return const_cast<NotificationManager *>(&notificationManager_);
    }

    HotReloadManager *GetHotReloadManager() const
    {
        return const_cast<HotReloadManager *>(&hotReloadManager_);
    }

    void SetDebugMode(bool isDebugMode)
    {
        if (isDebugMode_ == isDebugMode) {
            return;
        }

        isDebugMode_ = isDebugMode;

        if (isDebugMode) {
            jsThread_->SetDebugModeState();
        } else {
            jsThread_->ResetDebugModeState();
        }
    }

    bool IsDebugMode() const
    {
        return isDebugMode_;
    }

    void SetMixedDebugEnabled(bool enabled)
    {
        isMixedDebugEnabled_ = enabled;
    }

    bool IsMixedDebugEnabled() const
    {
        return isMixedDebugEnabled_;
    }

    void SetDebuggerHandler(ProtocolHandler *debuggerHandler)
    {
        debuggerHandler_ = debuggerHandler;
    }

    ProtocolHandler *GetDebuggerHandler() const
    {
        return debuggerHandler_;
    }

    void SetDebugLibraryHandle(LibraryHandle handle)
    {
        debuggerLibraryHandle_ = std::move(handle);
    }

    const LibraryHandle &GetDebugLibraryHandle() const
    {
        return debuggerLibraryHandle_;
    }

    void SetEvalFrameHandler(std::shared_ptr<FrameHandler> frameHandler)
    {
        frameHandler_ = frameHandler;
    }

    const std::shared_ptr<FrameHandler> &GetEvalFrameHandler() const
    {
        return frameHandler_;
    }

    void SetLocalScopeUpdater(ObjectUpdaterFunc *updaterFunc)
    {
        updaterFunc_ = updaterFunc;
    }

    void NotifyLocalScopeUpdated(std::string_view varName, Local<JSValueRef> value)
    {
        if (updaterFunc_ != nullptr) {
            (*updaterFunc_)(frameHandler_.get(), varName, value);
        }
    }

    void SetStepperFunc(SingleStepperFunc *stepperFunc)
    {
        stepperFunc_ = stepperFunc;
    }

    void ClearSingleStepper()
    {
        if (stepperFunc_ != nullptr) {
            (*stepperFunc_)();
        }
    }

    void MethodEntry(JSHandle<Method> method, JSHandle<JSTaggedValue> envHandle)
    {
        dropframeManager_.MethodEntry(jsThread_, method, envHandle);
    }

    void MethodExit(JSHandle<Method> method)
    {
        dropframeManager_.MethodExit(jsThread_, method);
    }

    void DropLastFrame()
    {
        dropframeManager_.DropLastFrame(jsThread_);
    }

    uint32_t GetPromiseQueueSizeRecordOfTopFrame()
    {
        return dropframeManager_.GetPromiseQueueSizeRecordOfTopFrame();
    }

private:
    bool isDebugMode_ {false};
    bool isMixedDebugEnabled_ { false };
    ProtocolHandler *debuggerHandler_ {nullptr};
    LibraryHandle debuggerLibraryHandle_ {nullptr};
    ObjectUpdaterFunc *updaterFunc_ {nullptr};
    SingleStepperFunc *stepperFunc_ {nullptr};
    JSThread *jsThread_ {nullptr};
    std::shared_ptr<FrameHandler> frameHandler_;
    DropframeManager dropframeManager_ { };

    NotificationManager notificationManager_;
    HotReloadManager hotReloadManager_;
};
}  // panda::ecmascript::tooling

#endif  // ECMASCRIPT_DEBUGGER_JS_DEBUGGER_MANAGER_H