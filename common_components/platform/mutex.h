/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef COMMON_COMPONENTS_PLATFORM_MUTEX_H
#define COMMON_COMPONENTS_PLATFORM_MUTEX_H
#ifdef DEBUG
#define FATAL_IF_ERROR(f, rc)                             \
    do {                                                  \
        if (rc != 0) {                                    \
            LOG_COMMON(FATAL)<< f << " failed: " << rc;   \
        }                                                 \
    } while (false)
#else
#define FATAL_IF_ERROR(f, rc) static_cast<void>(0)
#endif
#include "thread/thread_state_transition.h"

namespace common {
class Mutex {
public:
    explicit Mutex(bool isInit = true);

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

inline Mutex::Mutex(bool isInit) : mutex_()
{
    if (isInit) {
        Init(nullptr);
    }
}

inline Mutex::~Mutex()
{
    [[maybe_unused]] int rc = pthread_mutex_destroy(&mutex_);
    FATAL_IF_ERROR("pthread_mutex_destroy", rc);
}

inline void Mutex::Init(pthread_mutexattr_t *attrs)
{
    [[maybe_unused]] int rc = pthread_mutex_init(&mutex_, attrs);
    FATAL_IF_ERROR("pthread_mutex_init", rc);
}

inline void Mutex::Lock()
{
    [[maybe_unused]] int rc = pthread_mutex_lock(&mutex_);
    FATAL_IF_ERROR("pthread_mutex_lock", rc);
}

inline bool Mutex::TryLock()
{
    int rc = pthread_mutex_trylock(&mutex_);
    if (rc == EBUSY) {
        return false;
    }

    FATAL_IF_ERROR("pthread_mutex_trylock", rc);

    return true;
}

inline void Mutex::Unlock()
{
    [[maybe_unused]] int rc = pthread_mutex_unlock(&mutex_);
    FATAL_IF_ERROR("pthread_mutex_unlock", rc);
}
}
#endif //COMMON_COMPONENTS_PLATFORM_MUTEX_H
