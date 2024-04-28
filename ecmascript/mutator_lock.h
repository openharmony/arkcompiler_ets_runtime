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

#ifndef ECMASCRIPT_MUTATOR_LOCK_H
#define ECMASCRIPT_MUTATOR_LOCK_H

#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {

class MutatorLock : public RWLock {
#ifndef NDEBUG
public:
    enum MutatorLockState { UNLOCKED, RDLOCK, WRLOCK };
    void ReadLock();
    void WriteLock();
    bool TryReadLock();
    bool TryWriteLock();
    void Unlock();
    bool HasLock() const;

private:
    MutatorLockState GetState() const;
    void SetState(MutatorLockState newState);
#endif
};

class SuspendBarrier {
public:
    SuspendBarrier() : passBarrierCount_(0)
    {
    }

    explicit SuspendBarrier(int32_t count) : passBarrierCount_(count)
    {
    }

    void Wait();

    void Pass()
    {
        PassCount(-1);
    }

    void PassStrongly()
    {
        passBarrierCount_.fetch_sub(1, std::memory_order_seq_cst);
    }

    void Initialize(int32_t count)
    {
        passBarrierCount_.store(count, std::memory_order_relaxed);
    }

    void Increment(int32_t delta)
    {
        PassCount(delta);
        Wait();
    }

private:
    void PassCount(int32_t delta);
    std::atomic<int32_t> passBarrierCount_;
};

}  // namespace panda::ecmascript
#endif // ECMASCRIPT_MUTATOR_LOCK_H