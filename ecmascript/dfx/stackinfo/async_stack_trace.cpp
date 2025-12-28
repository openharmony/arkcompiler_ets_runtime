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

#include <iomanip>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/dfx/stackinfo/async_stack_trace.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/platform/async_detect.h"
#include "ecmascript/platform/backtrace.h"

#if defined(ENABLE_ASYNC_STACK)
#include "async_stack.h"
#include "unique_stack_table.h"
#endif

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

#if ENABLE_LATEST_OPTIMIZATION
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
        } else if (handler.IsJSAsyncModuleFulfilledFunction()) {
            JSTaggedValue module = JSAsyncModuleFulfilledFunction::Cast(handler)->GetModule(jsThread_);
            JSTaggedValue promise = SourceTextModule::Cast(module)->GetTopLevelCapability(jsThread_);
            asyncTaskId = JSPromise::Cast(promise)->GetAsyncTaskId();
        } else if (handler.IsJSAsyncModuleRejectedFunction()) {
            JSTaggedValue module = JSAsyncModuleRejectedFunction::Cast(handler)->GetModule(jsThread_);
            JSTaggedValue promise = SourceTextModule::Cast(module)->GetTopLevelCapability(jsThread_);
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
#else // ENABLE_LATEST_OPTIMIZATION
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
#endif // ENABLE_LATEST_OPTIMIZATION

bool AsyncStackTrace::RemoveAsyncTaskStack(const JSTaggedValue &PromiseReaction)
{
    if (PromiseReaction.IsPromiseReaction() && !currentAsyncParent_.empty()) {
        currentAsyncParent_.pop_back();
    }
    return true;
}

void AsyncStackTraceManager::SetAsyncStackType()
{
#if defined(ENABLE_ASYNC_STACK)
    defaultAsyncStackType_ = DfxSetAsyncStackType(ASYNC_TYPE_PROMISE);
    DfxSetAsyncStackType(defaultAsyncStackType_ | ASYNC_TYPE_PROMISE);
#endif
}

void AsyncStackTraceManager::ResetAsyncStackType()
{
#if defined(ENABLE_ASYNC_STACK)
    if (defaultAsyncStackType_ != 0) {
        DfxSetAsyncStackType(defaultAsyncStackType_);
    }
#endif
}

void AsyncStackTraceManager::SavePromiseNode(const JSHandle<JSPromise> &promise)
{
#if defined(ENABLE_ASYNC_STACK)
    CollectOldPromiseNodeIfNeeded();
    uint64_t stackId = DfxCollectStackWithDepth(ASYNC_TYPE_PROMISE, MAX_CALL_STACK_SIZE_TO_CAPTURE);
    if (stackId == 0) {
        LOG_ECMA(DEBUG) << "DFX collect async stack failed, return 0";
        return;
    }
    uint32_t promiseId = promise->GetAsyncTaskId();
    promiseMap_[promiseId] = {promiseId, GetCurrentPromiseId(), stackId};
    promiseQueue_.push_back(promiseId);
#endif
}

uint32_t AsyncStackTraceManager::GetParentPromiseId(uint32_t promiseId)
{
    auto it = promiseMap_.find(promiseId);
    if (it != promiseMap_.end()) {
        return it->second.parentPromiseId;
    }
    return 0;
}

uint64_t AsyncStackTraceManager::GetStackId(uint32_t promiseId)
{
    auto it = promiseMap_.find(promiseId);
    if (it != promiseMap_.end()) {
        return it->second.stackId;
    }
    return 0;
}

JSTaggedValue AsyncStackTraceManager::GetPromise(const JSTaggedValue &value)
{
    JSTaggedValue promise = JSTaggedValue::Hole();
    if (value.IsPromiseReaction()) {
        auto reaction = PromiseReaction::Cast(value);
        auto handler = reaction->GetHandler(thread_);
        if (handler.IsJSAsyncAwaitStatusFunction()) {
            // async await
            JSTaggedValue ctx = JSAsyncAwaitStatusFunction::Cast(handler)->GetAsyncContext(thread_);
            JSTaggedValue asyncFuncObj = GeneratorContext::Cast(ctx)->GetGeneratorObject(thread_);
            promise = JSAsyncFuncObject::Cast(asyncFuncObj)->GetPromise(thread_);
        } else if (handler.IsJSAsyncModuleFulfilledFunction()) {
            // top-level await fulfill
            JSTaggedValue module = JSAsyncModuleFulfilledFunction::Cast(handler)->GetModule(thread_);
            promise = SourceTextModule::Cast(module)->GetTopLevelCapability(thread_);
        } else if (handler.IsJSAsyncModuleRejectedFunction()) {
            // top-level await reject
            JSTaggedValue module = JSAsyncModuleRejectedFunction::Cast(handler)->GetModule(thread_);
            promise = SourceTextModule::Cast(module)->GetTopLevelCapability(thread_);
        } else {
            JSTaggedValue promiseOrCapability = reaction->GetPromiseOrCapability(thread_);
            if (promiseOrCapability.IsJSPromise()) {
                // builtin promise
                promise = promiseOrCapability;
            } else {
                // promise like
                promise = PromiseCapability::Cast(promiseOrCapability)->GetPromise(thread_);
            }
        }
    }
    if (value.IsJSPromiseReactionFunction()) {
        // dynamic import
        promise = JSPromiseReactionsFunction::Cast(value)->GetPromise(thread_);
    }
    return promise;
}

void AsyncStackTraceManager::SetCurrentPromiseTask(const JSTaggedValue &value)
{
#if defined(ENABLE_ASYNC_STACK)
    JSTaggedValue promise = GetPromise(value);
    if (!promise.IsHole()) {
        currentPromiseId_ = JSPromise::Cast(promise)->GetAsyncTaskId();
        uint64_t stackId = GetStackId(currentPromiseId_);
        if (stackId != 0) {
            DfxSetSubmitterStackId(stackId);
        }
    }
#endif
}

void AsyncStackTraceManager::ResetCurrentPromiseJob(const JSTaggedValue &value)
{
#if defined(ENABLE_ASYNC_STACK)
    if (value.IsPromiseReaction() || value.IsJSPromiseReactionFunction()) {
        currentPromiseId_ = 0;
        DfxSetSubmitterStackId(0);
    }
#endif
}

void AsyncStackTraceManager::CollectOldPromiseNodeIfNeeded()
{
    // Clear the outdated data
    if (promiseMap_.size() <= MAX_ASYNC_CALL_STACKS) return;
    size_t half = promiseMap_.size() / 2;
    for (size_t i = 0; i < half; ++i) {
        uint32_t promiseId = promiseQueue_.front();
        promiseQueue_.pop_front();
        promiseMap_.erase(promiseId);
    }
}

void AsyncStackTraceManager::BuildAsyncStackTrace(std::string &asyncStackTrace)
{
#if defined(ENABLE_ASYNC_STACK)
    uint32_t depth = 0;
    uint32_t promiseId = GetCurrentPromiseId();

    while (promiseId != 0 && depth < MAX_ASYNC_TASK_STACK_DEPTH) {
        ++depth;
        uint64_t stackId = GetStackId(promiseId);
        if (stackId == 0) {
            LOG_ECMA(DEBUG) << "StackId of the current promise is 0";
            promiseId = GetParentPromiseId(promiseId);
            continue;
        }

        std::vector<uintptr_t> pcs;
        OHOS::HiviewDFX::StackId id;
        id.value = stackId;
        if (!OHOS::HiviewDFX::UniqueStackTable::Instance()->GetPcsByStackId(id, pcs)) {
            LOG_ECMA(ERROR) << "Failed to get pcs by stackId";
            promiseId = GetParentPromiseId(promiseId);
            continue;
        }
        std::string stackStr = SymbolicAddress(
            reinterpret_cast<const void* const *>(pcs.data()),
            pcs.size(),
            vm_);

        std::ostringstream banner;
        banner << "submitter#" << std::setw(2) << std::setfill('0') << (depth - 1);
        asyncStackTrace.append(banner.str() + ":\n");
        asyncStackTrace.append(stackStr);

        promiseId = GetParentPromiseId(promiseId);
    }
#endif
}

void AsyncStackTraceManager::Clear()
{
    promiseQueue_.clear();
    promiseMap_.clear();
}
}  // namespace panda::ecmascript
