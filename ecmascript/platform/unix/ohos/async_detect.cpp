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

#include "ecmascript/platform/async_detect.h"

#include "ecmascript/dfx/stackinfo/async_stack_trace.h"

namespace panda::ecmascript {
void RegisterAsyncDetectCallBack([[maybe_unused]] EcmaVM *vm)
{
#if !defined(STANDALONE_MODE) && !defined(CROSS_PLATFORM)
    if (vm->GetJSOptions().EnablePendingCheak()) {
        uv_loop_t *loop = reinterpret_cast<uv_loop_t *>(vm->GetLoop());
        if (loop == nullptr) {
            LOG_ECMA(ERROR) << "Get loop failed";
            return;
        }
        static thread_local uv_timer_t handle;
        uv_timer_init(loop, &handle);
        handle.data = vm->GetAsyncStackTrace();
        uv_timer_start(&handle, AsyncDetectCallBack, 0, AsyncStackTrace::PROMISE_PENDING_TIME_MS);
        if (vm->GetJSThread()->IsMainThread()) {
            uv_async_send(&loop->wq_async);
        } else {
            uv_work_t *work = new uv_work_t;
            uv_queue_work(loop, work, [](uv_work_t *) { }, [](uv_work_t *work, int32_t) { delete work; });
        }
    }
#endif
}

#if !defined(STANDALONE_MODE) && !defined(CROSS_PLATFORM)
void AsyncDetectCallBack(uv_timer_t* handle)
{
    AsyncStackTrace *asyncStackTrace = static_cast<AsyncStackTrace *>(handle->data);
    auto now = std::chrono::system_clock::now();
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    CMap<uint32_t, std::pair<std::string, int64_t>> asyncTasks = asyncStackTrace->GetAsyncStackTrace();
    for (auto asyncTask : asyncTasks) {
        if (currentTime - asyncTask.second.second >= AsyncStackTrace::PROMISE_PENDING_TIME_MS) {
            LOG_ECMA(ERROR) << "Pending time: " << currentTime - asyncTask.second.second
                << ", stack: \n" <<  asyncTask.second.first;
        }
    }
}
#endif
} // namespace panda::ecmascript