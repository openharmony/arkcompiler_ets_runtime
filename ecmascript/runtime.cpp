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

#include "ecmascript/runtime.h"

namespace panda::ecmascript {

static Runtime *runtime = nullptr;

Runtime *Runtime::GetInstance()
{
    ASSERT(runtime != nullptr);
    return runtime;
}

void Runtime::CreateRuntimeIfNotCreated()
{
    static Mutex creationLock;
    LockHolder lock(creationLock);
    if (runtime == nullptr) {
        runtime = new Runtime();
    }
}

void Runtime::Destroy()
{
    if (runtime != nullptr) {
        delete runtime;
        runtime = nullptr;
    }
}

void Runtime::RegisterThread(JSThread* newThread)
{
    LockHolder lock(threadsLock_);
    threads_.emplace_back(newThread);
    // send all current suspended requests to the new thread
    for (uint32_t i = 0; i < suspendNewCount_; i++) {
        newThread->SuspendThread(true);
    }
}

// Note: currently only called when thread is to be destroyed.
void Runtime::UnregisterThread(JSThread* thread)
{
    LockHolder lock(threadsLock_);
    ASSERT(thread->GetState() != ThreadState::RUNNING);
    threads_.remove(thread);
}

void Runtime::SuspendAll(JSThread *current)
{
    ASSERT(current != nullptr);
    ASSERT(current->GetState() != ThreadState::RUNNING);
    SuspendAllThreadsImpl(current);
    mutatorLock_.WriteLock();
}

void Runtime::ResumeAll(JSThread *current)
{
    ASSERT(current != nullptr);
    ASSERT(current->GetState() != ThreadState::RUNNING);
    mutatorLock_.Unlock();
    ResumeAllThreadsImpl(current);
}

void Runtime::SuspendAllThreadsImpl(JSThread *current)
{
    LockHolder lock(threadsLock_);
    suspendNewCount_++;
    for (auto i : threads_) {
        if (i != current) {
            i->SuspendThread(true);
        }
    }
}

void Runtime::ResumeAllThreadsImpl(JSThread *current)
{
    LockHolder lock(threadsLock_);
    if (suspendNewCount_ > 0) {
        suspendNewCount_--;
    }
    for (auto i : threads_) {
        if (i != current) {
            i->ResumeThread(true);
        }
    }
}

}  // namespace panda::ecmascript
