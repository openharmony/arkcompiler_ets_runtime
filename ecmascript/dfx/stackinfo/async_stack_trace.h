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

#include <string>
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"

namespace panda::ecmascript {
class JSPromise;
class AsyncStackTrace {
public:
    explicit AsyncStackTrace(EcmaVM *vm);

    ~AsyncStackTrace() = default;

    void RegisterAsyncDetectCallBack();

    bool InsertAsyncStackTrace(const JSHandle<JSPromise> &promise);

    bool RemoveAsyncStackTrace(const JSHandle<JSPromise> &promise);

    CMap<uint32_t, std::pair<std::string, int64_t>> GetAsyncStackTrace()
    {
        return asyncStackTrace_;
    }

    uint32_t GetAsyncTaskId()
    {
        if (asyncTaskId_ == UINT32_MAX) {
            asyncTaskId_ = 0;
        }
        return ++asyncTaskId_;
    }

    static constexpr uint32_t PROMISE_PENDING_TIME_MS = 5000;

    NO_COPY_SEMANTIC(AsyncStackTrace);
    NO_MOVE_SEMANTIC(AsyncStackTrace);
private:
    EcmaVM *vm_ {nullptr};
    JSThread *jsThread_ {nullptr};
    uint32_t asyncTaskId_ {0};

    // { promiseid , (jsStack, time) }
    CMap<uint32_t, std::pair<std::string, int64_t>> asyncStackTrace_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_STACKINFO_STACK_TRACE_H