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

#ifndef ECMASCRIPT_RUNTIME_H
#define ECMASCRIPT_RUNTIME_H

#include "ecmascript/js_thread.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/mutator_lock.h"
#include "libpandabase/macros.h"
#include <list>

namespace panda::ecmascript {
class Runtime {
public:
    static Runtime *GetInstance();
    static void CreateRuntimeIfNotCreated();
    void Destroy();
    ~Runtime() = default;

    void RegisterThread(JSThread* newThread);
    void UnregisterThread(JSThread* thread);

    void SuspendAll(JSThread *current);
    void ResumeAll(JSThread *current);

    MutatorLock *GetMutatorLock()
    {
        return &mutatorLock_;
    }

    const MutatorLock *GetMutatorLock() const
    {
        return &mutatorLock_;
    }

private:
    Runtime() = default;
    void SuspendAllThreadsImpl(JSThread *current);
    void ResumeAllThreadsImpl(JSThread *current);

    Mutex threadsLock_;
    std::list<JSThread*> threads_;
    uint32_t suspendNewCount_ {0};

    MutatorLock mutatorLock_;

    NO_COPY_SEMANTIC(Runtime);
    NO_MOVE_SEMANTIC(Runtime);

    friend class JSThread;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_RUNTIME_H
