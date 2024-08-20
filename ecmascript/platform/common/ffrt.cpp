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

#include "ecmascript/platform/ffrt.h"
#include "libpandabase/os/thread.h"

#if defined(ENABLE_FFRT_INTERFACES)
#include "ffrt.h"
#include "c/executor_task.h"
#endif

namespace panda::ecmascript {
ThreadId GetCurrentThreadOrTaskId()
{
#if defined(ENABLE_FFRT_INTERFACES)
    ThreadId id = ffrt_this_task_get_id();
    if (id != 0) {
        return id;
    } else {
        return os::thread::GetCurrentThreadId();
    }
#else
    return os::thread::GetCurrentThreadId();
#endif
}

ThreadId GetThreadIdOrCachedTaskId()
{
#if defined(ENABLE_FFRT_INTERFACES)
    // If the current task id is zero,
    // ffrt_get_cur_cached_task_id() returns the id of the last task that ran in this thread
    ThreadId id = ffrt_get_cur_cached_task_id();
    if (id != 0) {
        return id;
    } else {
        return os::thread::GetCurrentThreadId();
    }
#else
    return os::thread::GetCurrentThreadId();
#endif
}
} // namespace panda::ecmascript

