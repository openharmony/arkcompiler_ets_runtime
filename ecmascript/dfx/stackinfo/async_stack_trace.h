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

#ifndef ECMASCRIPT_STACKINFO_STACK_TRACE_H
#define ECMASCRIPT_STACKINFO_STACK_TRACE_H

#include <memory>
#include <string>

#include "ecmascript/js_promise.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::test {
class AsyncStackTestHelper;
}

namespace panda::ecmascript {
class JSPromise;
class EcmaVM;
class JSTaggedValue;

// for async stack trace
static constexpr int32_t MAX_CALL_STACK_SIZE_TO_CAPTURE = 200;
class PUBLIC_API StackFrame {
public:
    StackFrame() = default;
    ~StackFrame() = default;

    const std::string &GetFunctionName() const
    {
        return functionName_;
    }

    void SetFunctionName(std::string &functionName)
    {
        functionName_ = functionName;
    }

    const std::string &GetUrl() const
    {
        return url_;
    }

    void SetUrl(std::string &url)
    {
        url_ = url;
    }

    int32_t GetLineNumber() const
    {
        return lineNumber_;
    }

    void SetLineNumber(int32_t lineNumber)
    {
        lineNumber_ = lineNumber;
    }

    int32_t GetColumnNumber() const
    {
        return columnNumber_;
    }

    void SetColumnNumber(int32_t columnNumber)
    {
        columnNumber_ = columnNumber;
    }

    int32_t GetScriptId() const
    {
        return scriptId_;
    }

    void SetScriptId(int32_t scriptId)
    {
        scriptId_ = scriptId;
    }

private:
    std::string functionName_;
    std::string url_;
    int32_t lineNumber_;
    int32_t columnNumber_;
    // For debugger
    int32_t scriptId_;
};

class AsyncStack {
public:
    AsyncStack() = default;

    const std::string &GetDescription() const
    {
        return description_;
    }

    void SetDescription(const std::string &description)
    {
        description_ = description;
    }

    const std::weak_ptr<AsyncStack> &GetAsyncParent() const
    {
        return asyncParent_;
    }

    void SetAsyncParent(const std::shared_ptr<AsyncStack> &asyncParent)
    {
        asyncParent_ = asyncParent;
    }

    const std::vector<std::shared_ptr<StackFrame>> &GetFrames() const
    {
        return frames_;
    }

    void SetFrames(const std::vector<std::shared_ptr<StackFrame>> &frames)
    {
        frames_ = frames;
    }
private:
    std::string description_;
    std::vector<std::shared_ptr<StackFrame>> frames_;
    std::weak_ptr<AsyncStack> asyncParent_;
};

class AsyncStackTrace {
public:
    explicit AsyncStackTrace(EcmaVM *vm);

    ~AsyncStackTrace() = default;

    void RegisterAsyncDetectCallBack();

    bool InsertAsyncStackTrace(const JSHandle<JSPromise> &promise);

    bool RemoveAsyncStackTrace(const JSHandle<JSPromise> &promise);

    bool InsertAsyncTaskStacks(const JSHandle<JSPromise> &promise, const std::string &description);

    bool InsertCurrentAsyncTaskStack(const JSTaggedValue &PromiseReaction);

    bool RemoveAsyncTaskStack(const JSTaggedValue &PromiseReaction);

    std::shared_ptr<AsyncStack> GetCurrentAsyncParent();

    CMap<uint32_t, std::pair<std::string, int64_t>> GetAsyncStackTrace()
    {
        return asyncStackTrace_;
    }

    uint32_t GetAsyncTaskId()
    {
        if (asyncTaskId_ == MAX_ASYNC_TASK_ID) {
            asyncTaskId_ = 0;
        }
        return ++asyncTaskId_;
    }

    static constexpr uint32_t PROMISE_PENDING_TIME_MS = 5000;

    NO_COPY_SEMANTIC(AsyncStackTrace);
    NO_MOVE_SEMANTIC(AsyncStackTrace);
private:
    static constexpr uint32_t MAX_ASYNC_TASK_ID = (1u << JSPromise::ASYNC_TASK_ID_BITS) - 1;

    EcmaVM *vm_ {nullptr};
    JSThread *jsThread_ {nullptr};
    uint32_t asyncTaskId_ {0};

    // { promiseid , (jsStack, time) }
    CMap<uint32_t, std::pair<std::string, int64_t>> asyncStackTrace_;

    std::vector<std::shared_ptr<AsyncStack>> currentAsyncParent_;
    CMap<uint32_t, std::shared_ptr<AsyncStack>> asyncTaskStacks_;
};

struct PromiseNode {
    uint32_t promiseId;
    uint32_t parentPromiseId;
    uint64_t stackId;
};

class AsyncStackTraceManager {
public:
    static constexpr uint32_t MAX_ASYNC_TASK_STACK_DEPTH = 8;

    explicit AsyncStackTraceManager(EcmaVM *vm) : vm_(vm)
    {
        thread_ = vm_->GetJSThread();
    }
    ~AsyncStackTraceManager() = default;

    void SetAsyncStackType();
    void ResetAsyncStackType();

    void SavePromiseNode(const JSHandle<JSPromise> &promise);
    uint32_t GetParentPromiseId(uint32_t promiseId);
    uint64_t GetStackId(uint32_t promiseId);
    void SetCurrentPromiseTask(const JSTaggedValue &value);
    void ResetCurrentPromiseJob(const JSTaggedValue &value);
    uint32_t GetCurrentPromiseId() const
    {
        return currentPromiseId_;
    }
    void BuildAsyncStackTrace(std::string &asyncStackTrace);
    void Clear();

private:
    static constexpr uint32_t MAX_ASYNC_CALL_STACKS = 4 * 1024;
    static constexpr uint32_t MAX_CALL_STACK_SIZE_TO_CAPTURE = 48;

    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    CDeque<uint32_t> promiseQueue_;
    CUnorderedMap<uint32_t, PromiseNode> promiseMap_;
    uint32_t currentPromiseId_ {0};
    uint64_t defaultAsyncStackType_ {0};

    void CollectOldPromiseNodeIfNeeded();
    JSTaggedValue GetPromise(const JSTaggedValue &value);

    friend class test::AsyncStackTestHelper;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_STACKINFO_STACK_TRACE_H