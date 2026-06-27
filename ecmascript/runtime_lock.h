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

#include "ecmascript/js_thread.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
namespace panda::ecmascript {

static inline void RuntimeLock(JSThread *thread, Mutex &mtx)
{
    if (mtx.TryLock()) {
        return;
    }
    ThreadStateTransitionScope<JSThread, ThreadState::WAIT> ts(thread);
    mtx.Lock();
}

static inline void RuntimeLock(DaemonThread *dThread, Mutex &mtx)
{
    if (mtx.TryLock()) {
        return;
    }
    ThreadStateTransitionScope<DaemonThread, ThreadState::WAIT> ts(dThread);
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

static inline void RuntimeReadLock(JSThread *thread, RWLock &lock)
{
    if (lock.TryReadLock()) {
        return;
    }
    ThreadStateTransitionScope<JSThread, ThreadState::WAIT> ts(thread);
    lock.ReadLock();
}

class RuntimeReadLockHolder {
public:
    RuntimeReadLockHolder(JSThread *thread, RWLock &lock) : lock_(lock)
    {
        RuntimeReadLock(thread, lock_);
    }

    ~RuntimeReadLockHolder()
    {
        lock_.Unlock();
    }

private:
    RWLock &lock_;

    NO_COPY_SEMANTIC(RuntimeReadLockHolder);
    NO_MOVE_SEMANTIC(RuntimeReadLockHolder);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_RUNTIME_LOCK_H
