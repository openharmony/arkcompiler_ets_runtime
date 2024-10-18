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

#ifndef ECMASCRIPT_PLATFORM_FFRT_MUTEX_H
#define ECMASCRIPT_PLATFORM_FFRT_MUTEX_H

#include "ffrt.h"

#include "ecmascript/common.h"
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

class PUBLIC_API FFRTMutex {
public:
    explicit FFRTMutex(bool is_init = true)
    {
        if (is_init) {
            Init(nullptr);
        }
    }

    ~FFRTMutex()
    {
        [[maybe_unused]]int rc  = ffrt_mutex_destroy(&mutex_);
        FATAL_IF_ERROR("ffrt_mutex_destroy", rc);
    }

    void Lock()
    {
        [[maybe_unused]]int rc = ffrt_mutex_lock(&mutex_);
        FATAL_IF_ERROR("ffrt_mutex_lock", rc);
    }

    bool TryLock()
    {
        int rc = ffrt_mutex_trylock(&mutex_);
        if (rc == EBUSY) {
            return false;
        }

        FATAL_IF_ERROR("ffrt_mutex_trylock", rc);

        return true;
    }

    void Unlock()
    {
        [[maybe_unused]]int rc = ffrt_mutex_unlock(&mutex_);
        FATAL_IF_ERROR("ffrt_mutex_unlock", rc);
    }

protected:
    void Init(ffrt_mutexattr_t *attrs)
    {
        [[maybe_unused]]int rc = ffrt_mutex_init(&mutex_, attrs);
        FATAL_IF_ERROR("ffrt_mutex_init", rc);
    }

private:
    ffrt_mutex_t mutex_;
    friend class FFRTConditionVariable;
};

class PUBLIC_API FFRTConditionVariable {
public:
    FFRTConditionVariable()
    {
        [[maybe_unused]]int rc = ffrt_cond_init(&cond_, nullptr);
        FATAL_IF_ERROR("ffrt_cond_init", rc);
    }

    ~FFRTConditionVariable()
    {
        [[maybe_unused]]int rc = ffrt_cond_destroy(&cond_);
        FATAL_IF_ERROR("ffrt_cond_destroy", rc);
    }

    void Signal()
    {
        [[maybe_unused]]int rc = ffrt_cond_signal(&cond_);
        FATAL_IF_ERROR("ffrt_cond_signal", rc);
    }

    void SignalAll()
    {
        [[maybe_unused]]int rc = ffrt_cond_broadcast(&cond_);
        FATAL_IF_ERROR("ffrt_cond_broadcast", rc);
    }

    void Wait(FFRTMutex *mutex)
    {
        [[maybe_unused]]int rc = ffrt_cond_wait(&cond_, &mutex->mutex_);
        FATAL_IF_ERROR("ffrt_cond_wait", rc);
    }

    bool TimedWait(FFRTMutex *mutex, uint64_t ms, uint64_t ns = 0, bool is_absolute = false)
    {
        struct timespec abs_time = ConvertTime(ms, ns, is_absolute);
        int rc = ffrt_cond_timedwait(&cond_, &mutex->mutex_, &abs_time);
        if (rc != 0) {
            if (rc == ETIMEDOUT) {
                // interrupted
                return true;
            }
        }
        FATAL_IF_ERROR("ffrt_cond_timedwait", rc);
        return false;
    }

protected:
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

private:
    ffrt_cond_t cond_;
};

class FFRTLockHolder {
public:
    explicit FFRTLockHolder(FFRTMutex &mtx) : lock_(mtx)
    {
        lock_.Lock();
    }

    ~FFRTLockHolder()
    {
        lock_.Unlock();
    }

private:
    FFRTMutex &lock_;

    NO_COPY_SEMANTIC(FFRTLockHolder);
    NO_MOVE_SEMANTIC(FFRTLockHolder);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_FFRT_MUTEX_H