/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include <shared_mutex>
#include <unordered_map>

#include "ecmascript/debugger/hot_reload_manager.h"
#include "ecmascript/debugger/notification_manager.h"
#include "ecmascript/debugger/dropframe_manager.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/global_handle_collection.h"

#include "libpandabase/os/library_loader.h"

namespace panda::ecmascript::tooling {
class ProtocolHandler;
class JsDebuggerManager {
public:
    using LibraryHandle = os::library_loader::LibraryHandle;
    using ObjectUpdaterFunc =
        std::function<void(const FrameHandler *, std::string_view, Local<JSValueRef>, const std::string&)>;
    using SingleStepperFunc = std::function<void()>;
    using ReturnNativeFunc = std::function<void()>;

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

    void PUBLIC_API SetDebugMode(bool isDebugMode);

    bool IsDebugMode() const
    {
        return isDebugMode_;
    }

    void SetIsDebugApp(bool isDebugApp)
    {
        isDebugApp_ = isDebugApp;
    }

    bool IsDebugApp() const
    {
        return isDebugApp_;
    }

    void SetFaApp(bool isFaApp)
    {
        isFaApp_ = isFaApp;
    }
    
    bool GetFaApp() const
    {
        return isFaApp_;
    }

    void SetMixedStackEnabled(bool mixedStackEnabled)
    {
        isMixedStackEnabled_ = mixedStackEnabled;
    }

    bool IsMixedStackEnabled() const
    {
        return isMixedStackEnabled_;
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

    bool GetSignalState() const
    {
        return isSignalInterrupt_;
    }

    void SetSignalState(bool isSignalInterrupt)
    {
        isSignalInterrupt_ = isSignalInterrupt;
    }

    bool IsAsyncStackTrace() const
    {
        return asyncStackTrace_;
    }

    void SetAsyncStackTrace(bool asyncStackTrace)
    {
        asyncStackTrace_ = asyncStackTrace;
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

    void NotifyScopeUpdated(std::string_view varName, Local<JSValueRef> value, const std::string& scope)
    {
        if (updaterFunc_ != nullptr) {
            (*updaterFunc_)(frameHandler_.get(), varName, value, scope);
        }
    }

    void SetJSReturnNativeFunc(ReturnNativeFunc *returnNative)
    {
        returnNative_ = returnNative;
    }

    void NotifyReturnNative()
    {
        if (returnNative_ != nullptr) {
            (*returnNative_)();
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

    bool CheckIsSendableMethod()
    {
        return dropframeManager_.CheckIsSendableMethod();
    }

    void EnableObjectHashDisplay()
    {
        isObjHashDisplayEnabled_ = true;
    }

    void DisableObjectHashDisplay()
    {
        isObjHashDisplayEnabled_ = false;
    }

    bool IsObjHashDisplayEnabled()
    {
        return isObjHashDisplayEnabled_;
    }
    
    void EnableSerializationTimeoutCheck()
    {
        isSerializationTimeoutCheckEnabled_ = true;
    }

    void DisableSerializationTimeoutCheck()
    {
        isSerializationTimeoutCheckEnabled_ = false;
    }

    bool IsSerializationTimeoutCheckEnabled()
    {
        return isSerializationTimeoutCheckEnabled_;
    }

    void SetSerializationCheckThreshold(int threshold)
    {
        serializationCheckThreshold_ = threshold;
    }

    int32_t GetSerializationCheckThreshold() const
    {
        return serializationCheckThreshold_;
    }

    static void AddJsDebuggerManager(int tid, JsDebuggerManager *jsDebuggerManager);
    static JsDebuggerManager* GetJsDebuggerManager(int tid);
    static void DeleteJsDebuggerManager(int tid);

private:
    bool isDebugMode_ {false};
    bool isDebugApp_ {false};
    bool isFaApp_ {false};
    bool isMixedDebugEnabled_ { false };
    bool isMixedStackEnabled_ { false };
    bool isSignalInterrupt_ {false};
    bool isObjHashDisplayEnabled_ { true };
    bool asyncStackTrace_ {false};
    ProtocolHandler *debuggerHandler_ {nullptr};
    LibraryHandle debuggerLibraryHandle_ {nullptr};
    ObjectUpdaterFunc *updaterFunc_ {nullptr};
    SingleStepperFunc *stepperFunc_ {nullptr};
    ReturnNativeFunc *returnNative_ {nullptr};
    JSThread *jsThread_ {nullptr};
    std::shared_ptr<FrameHandler> frameHandler_ {nullptr};
    DropframeManager dropframeManager_;

    NotificationManager notificationManager_;
    HotReloadManager hotReloadManager_;
    // Serialization / DeSerialization Timeout flag
    bool isSerializationTimeoutCheckEnabled_ { false };
    // in milliseconds
    static constexpr int32_t DEFAULT_THRESHOLD = 8;
    int32_t serializationCheckThreshold_ { DEFAULT_THRESHOLD };

    static std::unordered_map<int, JsDebuggerManager *> jsDebuggerManagerMap_;
    static std::shared_mutex mutex_;
};
}  // panda::ecmascript::tooling

#endif  // ECMASCRIPT_DEBUGGER_JS_DEBUGGER_MANAGER_H
