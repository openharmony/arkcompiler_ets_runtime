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

#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mutator_lock.h"

#include "libpandabase/macros.h"

#include <list>
#include <memory>

namespace panda::ecmascript {
class Runtime {
public:
    static Runtime *GetInstance();

    static void CreateIfFirstVm(const JSRuntimeOptions &options);
    static void DestroyIfLastVm();
    void InitializeIfFirstVm(EcmaVM *vm);

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

    template<class Callback>
    void IterateThreadList(const Callback &cb)
    {
        LockHolder lock(threadsLock_);
        for (auto thread : threads_) {
            cb(thread);
        }
    }

    inline const GlobalEnvConstants *GetGlobalEnvConstants()
    {
        return globalConstants_;
    }

    inline EcmaStringTable *GetEcmaStringTable() const
    {
        return stringTable_.get();
    }

private:
    Runtime() = default;
    ~Runtime() = default;
    void SuspendAllThreadsImpl(JSThread *current);
    void ResumeAllThreadsImpl(JSThread *current);

    void PreInitialization(const EcmaVM *vm);
    void PostInitialization(const EcmaVM *vm);

    Mutex threadsLock_;
    std::list<JSThread*> threads_;
    uint32_t suspendNewCount_ {0};
    MutatorLock mutatorLock_;

    const GlobalEnvConstants *globalConstants_ {nullptr};
    JSThread *mainThread_ {nullptr};
    // for shared heap.
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<HeapRegionAllocator> heapRegionAllocator_;
    // for stringTable.
    std::unique_ptr<EcmaStringTable> stringTable_;

    // Runtime instance and VMs creation.
    static int32_t vmCount_;
    static bool firstVmCreated_;
    static Mutex *vmCreationLock_;
    static Runtime *instance_;

    friend class EcmaVM;
    friend class JSThread;

    NO_COPY_SEMANTIC(Runtime);
    NO_MOVE_SEMANTIC(Runtime);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_RUNTIME_H
