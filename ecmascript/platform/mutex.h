/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PLATFORM_MUTEX_H
#define ECMASCRIPT_PLATFORM_MUTEX_H

#include <pthread.h>
#include <atomic>

#include "ecmascript/common.h"

#if defined(PANDA_USE_FUTEX)
#include "ecmascript/platform/unix/futex.h"
#endif

#ifdef DEBUG
#define FATAL_IF_ERROR(f, rc)                           \
    do {                                                \
        if (rc != 0) {                                  \
            LOG_ECMA(FATAL)<< f << " failed: " << rc;   \
        }                                               \
    } while (false)
#else
#define FATAL_IF_ERROR(f, rc) static_cast<void>(0)
#endif
namespace panda::ecmascript {

class PUBLIC_API Mutex {
public:
    explicit  Mutex(bool is_init = true);

    ~Mutex();

    void Lock();

    bool TryLock();

    void Unlock();

protected:
    void Init(pthread_mutexattr_t *attrs);

private:
    pthread_mutex_t mutex_;
    friend class ConditionVariable;
};

class RecursiveMutex : public Mutex {
public:
    RecursiveMutex();

    ~RecursiveMutex() = default;

    NO_COPY_SEMANTIC(RecursiveMutex);
    NO_MOVE_SEMANTIC(RecursiveMutex);
};

class RWLock {
public:
    RWLock();

    ~RWLock();

    void ReadLock();

    void WriteLock();

    bool TryReadLock();

    bool TryWriteLock();

    void Unlock();

private:
    pthread_rwlock_t rwlock_;

    NO_COPY_SEMANTIC(RWLock);
    NO_MOVE_SEMANTIC(RWLock);
};

class PUBLIC_API ConditionVariable {
public:
    ConditionVariable();

    ~ConditionVariable();

    void Signal();

    void SignalAll();

    void Wait(Mutex *mutex);

    bool TimedWait(Mutex *mutex, uint64_t ms, uint64_t ns = 0, bool is_absolute = false);

private:
    pthread_cond_t cond_;
};

class LockHolder {
public:
    explicit LockHolder(Mutex &mtx) : lock_(mtx)
    {
        lock_.Lock();
    }

    ~LockHolder()
    {
        lock_.Unlock();
    }

private:
    Mutex &lock_;

    NO_COPY_SEMANTIC(LockHolder);
    NO_MOVE_SEMANTIC(LockHolder);
};

class ReadLockHolder {
public:
    explicit ReadLockHolder(RWLock &lock) : lock_(lock)
    {
        lock_.ReadLock();
    }

    ~ReadLockHolder()
    {
        lock_.Unlock();
    }

private:
    RWLock &lock_;

    NO_COPY_SEMANTIC(ReadLockHolder);
    NO_MOVE_SEMANTIC(ReadLockHolder);
};

class WriteLockHolder {
public:
    explicit WriteLockHolder(RWLock &lock) : lock_(lock)
    {
        lock_.WriteLock();
    }

    ~WriteLockHolder()
    {
        lock_.Unlock();
    }

private:
    RWLock &lock_;

    NO_COPY_SEMANTIC(WriteLockHolder);
    NO_MOVE_SEMANTIC(WriteLockHolder);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_MUTEX_H
