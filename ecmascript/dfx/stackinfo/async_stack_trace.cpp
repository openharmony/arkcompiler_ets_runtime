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
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/platform/async_detect.h"
#include "ecmascript/debugger/js_debugger_manager.h"

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
    asyncStackTrace_.emplace(promise->GetAsyncTaskId(), std::make_pair(stack, currentTime));
    return true;
}

bool AsyncStackTrace::RemoveAsyncStackTrace(const JSHandle<JSPromise> &promise)
{
    auto asyncStackTrace = asyncStackTrace_.find(promise->GetAsyncTaskId());
    if (asyncStackTrace == asyncStackTrace_.end()) {
        LOG_ECMA(INFO) << "Not find promise: " << promise->GetAsyncTaskId();
        return false;
    }
    asyncStackTrace_.erase(asyncStackTrace);
    return true;
}

std::shared_ptr<AsyncStack> AsyncStackTrace::GetCurrentAsyncParent()
{
    return currentAsyncParent_.empty() ? nullptr : currentAsyncParent_.back();
}

bool AsyncStackTrace::InsertAsyncTaskStacks(const JSHandle<JSPromise> &promise, const std::string &description)
{
    uint32_t asyncTaskId = promise->GetAsyncTaskId();
    std::shared_ptr<AsyncStack> asyncStack = std::make_shared<AsyncStack>();
    auto notificationManager = vm_->GetJsDebuggerManager()->GetNotificationManager();
    // await need to skip top frame
    if (description == "await") {
        notificationManager->GenerateAsyncFramesEvent(asyncStack, true);
    } else {
        notificationManager->GenerateAsyncFramesEvent(asyncStack, false);
    }
    asyncStack->SetDescription(description);
    std::shared_ptr<AsyncStack> currentAsyncParent = GetCurrentAsyncParent();
    if (currentAsyncParent) {
        asyncStack->SetAsyncParent(currentAsyncParent);
    }
    asyncTaskStacks_.emplace(asyncTaskId, asyncStack);
    return true;
}

#ifdef ENABLE_NEXT_OPTIMIZATION
bool AsyncStackTrace::InsertCurrentAsyncTaskStack(const JSTaggedValue &PromiseReaction)
{
    if (PromiseReaction.IsPromiseReaction()) {
        uint32_t asyncTaskId = 0;
        auto reaction = PromiseReaction::Cast(PromiseReaction);
        auto handler = reaction->GetHandler(jsThread_);
        if (handler.IsJSAsyncAwaitStatusFunction()) {
            JSTaggedValue ctx = JSAsyncAwaitStatusFunction::Cast(handler)->GetAsyncContext(jsThread_);
            JSTaggedValue asyncFuncObj = GeneratorContext::Cast(ctx)->GetGeneratorObject(jsThread_);
            JSTaggedValue promise = JSAsyncFuncObject::Cast(asyncFuncObj)->GetPromise(jsThread_);
            asyncTaskId = JSPromise::Cast(promise)->GetAsyncTaskId();
        } else {
            JSTaggedValue promiseOrCapability = reaction->GetPromiseOrCapability(jsThread_);
            if (promiseOrCapability.IsJSPromise()) {
                asyncTaskId = JSPromise::Cast(promiseOrCapability)->GetAsyncTaskId();
            } else {
                JSTaggedValue promise = PromiseCapability::Cast(promiseOrCapability)->GetPromise(jsThread_);
                asyncTaskId = JSPromise::Cast(promise)->GetAsyncTaskId();
            }
        }
        auto stackIt = asyncTaskStacks_.find(asyncTaskId);
        if (stackIt != asyncTaskStacks_.end()) {
            currentAsyncParent_.emplace_back(stackIt->second);
        } else {
            currentAsyncParent_.emplace_back();
            LOG_DEBUGGER(ERROR) << "Promise: " << asyncTaskId << " is not in asyncTaskStacks";
        }
    }
    return true;
}
#else // ENABLE_NEXT_OPTIMIZATION
bool AsyncStackTrace::InsertCurrentAsyncTaskStack(const JSTaggedValue &PromiseReaction)
{
    if (PromiseReaction.IsPromiseReaction()) {
        JSTaggedValue promiseCapability = PromiseReaction::Cast(PromiseReaction)->GetPromiseCapability(jsThread_);
        JSTaggedValue promise = PromiseCapability::Cast(promiseCapability)->GetPromise(jsThread_);
        uint32_t asyncTaskId = JSPromise::Cast(promise)->GetAsyncTaskId();
        auto stackIt = asyncTaskStacks_.find(asyncTaskId);
        if (stackIt != asyncTaskStacks_.end()) {
            currentAsyncParent_.emplace_back(stackIt->second);
        } else {
            currentAsyncParent_.emplace_back();
            LOG_DEBUGGER(ERROR) << "Promise: " << asyncTaskId << " is not in asyncTaskStacks";
        }
    }
    return true;
}
#endif // ENABLE_NEXT_OPTIMIZATION

bool AsyncStackTrace::RemoveAsyncTaskStack(const JSTaggedValue &PromiseReaction)
{
    if (PromiseReaction.IsPromiseReaction() && !currentAsyncParent_.empty()) {
        currentAsyncParent_.pop_back();
    }
    return true;
}
}  // namespace panda::ecmascript