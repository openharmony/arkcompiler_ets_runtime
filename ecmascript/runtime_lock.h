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

#ifndef ECMASCRIPT_RUNTIME_LOCK_H
#define ECMASCRIPT_RUNTIME_LOCK_H

#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
namespace panda::ecmascript {

// Manually manage lock implementation
static inline void RuntimeLock(JSThread *thread, Mutex &mtx)
{
    if (mtx.TryLock()) {
        return;
    }
#ifndef NDEBUG
    if (g_isEnableCMCGC) {
        common::BaseRuntime::RequestGC(common::GC_REASON_USER, true, common::GC_TYPE_FULL);  // Trigger CMC FULL GC
    } else {
        SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    }
#endif
    ThreadStateTransitionScope<JSThread, ThreadState::WAIT> ts(thread);
    mtx.Lock();
}

static inline void RuntimeUnLock(Mutex &mtx)
{
    mtx.Unlock();
}

class RuntimeLockHolder {
public:
    RuntimeLockHolder(JSThread *thread, Mutex &mtx);

    ~RuntimeLockHolder()
    {
        mtx_.Unlock();
    }

private:
    JSThread *thread_;
    Mutex &mtx_;

    NO_COPY_SEMANTIC(RuntimeLockHolder);
    NO_MOVE_SEMANTIC(RuntimeLockHolder);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_RUNTIME_LOCK_H
