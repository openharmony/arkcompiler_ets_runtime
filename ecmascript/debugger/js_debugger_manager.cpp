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

#include "ecmascript/debugger/js_debugger_manager.h"

namespace panda::ecmascript::tooling {

    std::unordered_map<uint32_t, JsDebuggerManager *> JsDebuggerManager::jsDebuggerManagerMap_ {};
    std::shared_mutex JsDebuggerManager::mutex_;

    void JsDebuggerManager::AddJsDebuggerManager (uint32_t tid, JsDebuggerManager *jsDebuggerManager)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        if (jsDebuggerManagerMap_.find(tid) == jsDebuggerManagerMap_.end()) {
            jsDebuggerManagerMap_.emplace(tid, jsDebuggerManager);
        }
    }

    JsDebuggerManager *JsDebuggerManager::GetJsDebuggerManager(uint32_t tid)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (jsDebuggerManagerMap_.find(tid) == jsDebuggerManagerMap_.end()) {
            return nullptr;
        }
        return jsDebuggerManagerMap_[tid];
    }
} // namespace panda::ecmascript::tooling