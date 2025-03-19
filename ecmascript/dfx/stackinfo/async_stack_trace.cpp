/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/stackinfo/async_stack_trace.h"

#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/platform/async_detect.h"

namespace panda::ecmascript {
AsyncStackTrace::AsyncStackTrace(EcmaVM *vm) : vm_(vm)
{
    jsThread_ = vm->GetJSThread();
}

void AsyncStackTrace::RegisterAsyncDetectCallBack()
{
    panda::ecmascript::RegisterAsyncDetectCallBack(vm_);
}

bool AsyncStackTrace::InsertAsyncStackTrace(const JSHandle<JSPromise> &promise)
{
    std::string stack = JsStackInfo::BuildJsStackTrace(jsThread_, false, false);
    if (stack.empty()) {
        return false;
    }
    auto now = std::chrono::system_clock::now();
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    asyncStackTrace_.emplace(static_cast<uint32_t>(promise->GetAsyncTaskId()), std::make_pair(stack, currentTime));
    return true;
}

bool AsyncStackTrace::RemoveAsyncStackTrace(const JSHandle<JSPromise> &promise)
{
    auto asyncStackTrace = asyncStackTrace_.find(static_cast<uint32_t>(promise->GetAsyncTaskId()));
    if (asyncStackTrace == asyncStackTrace_.end()) {
        LOG_ECMA(INFO) << "Not find promise: " << static_cast<uint32_t>(promise->GetAsyncTaskId());
        return false;
    }
    asyncStackTrace_.erase(asyncStackTrace);
    return true;
}
}  // namespace panda::ecmascript