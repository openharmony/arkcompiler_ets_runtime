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

#include "ecmascript/platform/mutex.h"


namespace panda::ecmascript {

Mutex::Mutex(bool is_init) : mutex_()
{
    if (is_init) {
        Init(nullptr);
    }
}

Mutex::~Mutex()
{
    [[maybe_unused]]int rc  = pthread_mutex_destroy(&mutex_);
    FATAL_IF_ERROR("pthread_mutex_destroy", rc);
}

void Mutex::Init(pthread_mutexattr_t *attrs)
{
    [[maybe_unused]]int rc = pthread_mutex_init(&mutex_, attrs);
    FATAL_IF_ERROR("pthread_mutex_init", rc);
}

void Mutex::Lock()
{
    [[maybe_unused]]int rc = pthread_mutex_lock(&mutex_);
    FATAL_IF_ERROR("pthread_mutex_lock", rc);
}

bool Mutex::TryLock()
{
    int rc = pthread_mutex_trylock(&mutex_);
    if (rc == EBUSY) {
        return false;
    }

    FATAL_IF_ERROR("pthread_mutex_trylock", rc);

    return true;
}

void Mutex::Unlock()
{
    [[maybe_unused]]int rc = pthread_mutex_unlock(&mutex_);
    FATAL_IF_ERROR("pthread_mutex_unlock", rc);
}

RecursiveMutex::RecursiveMutex() : Mutex(false)
{
    pthread_mutexattr_t attrs;
    pthread_mutexattr_init(&attrs);
    pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_RECURSIVE);
    Init(&attrs);
}

RWLock::RWLock() : rwlock_()
{
    [[maybe_unused]]int rc = pthread_rwlock_init(&rwlock_, nullptr);
    FATAL_IF_ERROR("pthread_rwlock_init", rc);
}

RWLock::~RWLock()
{
    [[maybe_unused]]int rc = pthread_rwlock_destroy(&rwlock_);
    FATAL_IF_ERROR("pthread_rwlock_destroy", rc);
}

void RWLock::ReadLock()
{
    [[maybe_unused]]int rc = pthread_rwlock_rdlock(&rwlock_);
    FATAL_IF_ERROR("pthread_rwlock_rdlock", rc);
}

void RWLock::WriteLock()
{
    [[maybe_unused]]int rc = pthread_rwlock_wrlock(&rwlock_);
    FATAL_IF_ERROR("pthread_rwlock_wrlock", rc);
}

bool RWLock::TryReadLock()
{
    int rc = pthread_rwlock_tryrdlock(&rwlock_);
    if (rc == EBUSY) {
        return false;
    }

    FATAL_IF_ERROR("pthread_rwlock_tryrdlock", rc);

    return true;
}

bool RWLock::TryWriteLock()
{
    int rc = pthread_rwlock_trywrlock(&rwlock_);
    if (rc == EBUSY) {
        return false;
    }

    FATAL_IF_ERROR("pthread_rwlock_trywrlock", rc);

    return true;
}

void RWLock::Unlock()
{
    [[maybe_unused]]int rc = pthread_rwlock_unlock(&rwlock_);
    FATAL_IF_ERROR("pthread_rwlock_unlock", rc);
}

ConditionVariable::ConditionVariable() : cond_()
{
    [[maybe_unused]]int rc = pthread_cond_init(&cond_, nullptr);
    FATAL_IF_ERROR("pthread_cond_init", rc);
}

ConditionVariable::~ConditionVariable()
{
    [[maybe_unused]]int rc = pthread_cond_destroy(&cond_);
    FATAL_IF_ERROR("pthread_cond_destroy", rc);
}

void ConditionVariable::Signal()
{
    [[maybe_unused]]int rc = pthread_cond_signal(&cond_);
    FATAL_IF_ERROR("pthread_cond_signal", rc);
}

void ConditionVariable::SignalAll()
{
    [[maybe_unused]]int rc = pthread_cond_broadcast(&cond_);
    FATAL_IF_ERROR("pthread_cond_broadcast", rc);
}

void ConditionVariable::Wait(Mutex *mutex)
{
    [[maybe_unused]]int rc = pthread_cond_wait(&cond_, &mutex->mutex_);
    FATAL_IF_ERROR("pthread_cond_wait", rc);
}

struct timespec ConvertTime(uint64_t ms, uint64_t ns, bool is_absolute)
{
    struct timespec abs_time = {0, 0};
    if (!is_absolute) {
        clock_gettime(CLOCK_REALTIME, &abs_time);
    }
    const int64_t MILLISECONDS_PER_SEC = 1000;
    const int64_t NANOSECONDS_PER_MILLISEC = 1000000;
    const int64_t NANOSECONDS_PER_SEC = 1000000000;
    time_t seconds = ms / MILLISECONDS_PER_SEC;
    time_t nanoseconds = (ms % MILLISECONDS_PER_SEC) * NANOSECONDS_PER_MILLISEC + ns;
    abs_time.tv_sec += seconds;
    abs_time.tv_nsec += nanoseconds;
    if (abs_time.tv_nsec >= NANOSECONDS_PER_SEC) {
        abs_time.tv_nsec -= NANOSECONDS_PER_SEC;
        abs_time.tv_sec++;
    }
    return abs_time;
}

bool ConditionVariable::TimedWait(Mutex *mutex, uint64_t ms, uint64_t ns, bool is_absolute)
{
    struct timespec abs_time = ConvertTime(ms, ns, is_absolute);
    int rc = pthread_cond_timedwait(&cond_, &mutex->mutex_, &abs_time);
    if (rc != 0) {
        if (rc == ETIMEDOUT) {
            // interrupted
            return true;
        }
    }
    FATAL_IF_ERROR("pthread_cond_timedwait", rc);
    return false;
}
}  // namespace panda::ecmascript