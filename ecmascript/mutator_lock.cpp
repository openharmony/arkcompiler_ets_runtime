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
 
#include "ecmascript/mutator_lock.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
#ifndef NDEBUG
void MutatorLock::ReadLock()
{
    ASSERT(!HasLock());
    RWLock::ReadLock();
    SetState(MutatorLockState::RDLOCK);
}

void MutatorLock::WriteLock()
{
    ASSERT(!HasLock());
    RWLock::WriteLock();
    SetState(MutatorLockState::WRLOCK);
}

bool MutatorLock::TryReadLock()
{
    ASSERT(!HasLock());
    bool res = RWLock::TryReadLock();
    if (res) {
        SetState(MutatorLockState::RDLOCK);
    }
    return res;
}

bool MutatorLock::TryWriteLock()
{
    ASSERT(!HasLock());
    bool res = RWLock::TryWriteLock();
    if (res) {
        SetState(MutatorLockState::WRLOCK);
    }
    return res;
}

void MutatorLock::Unlock()
{
    ASSERT(HasLock());
    RWLock::Unlock();
    SetState(MutatorLockState::UNLOCKED);
}

bool MutatorLock::HasLock() const
{
    return GetState() != MutatorLockState::UNLOCKED;
}

MutatorLock::MutatorLockState MutatorLock::GetState() const
{
    return JSThread::GetCurrent()->GetMutatorLockState();
}

void MutatorLock::SetState(MutatorLock::MutatorLockState newState)
{
    JSThread::GetCurrent()->SetMutatorLockState(newState);
}
#endif

void SuspendBarrier::Wait()
{
    while (true) {
        int32_t curCount = passBarrierCount_.load(std::memory_order_relaxed);
        if (LIKELY(curCount > 0)) {
#if defined(PANDA_TARGET_OHOS)
            sched_yield();
#endif
        } else {
            // Use seq_cst to synchronize memory.
            curCount = passBarrierCount_.load(std::memory_order_seq_cst);
            ASSERT(curCount == 0);
            break;
        }
    }
}

void SuspendBarrier::PassCount(int32_t delta)
{
    bool done = false;
    do {
        int32_t curCount = passBarrierCount_.load(std::memory_order_relaxed);
        // Reduce value by 1.
        done = passBarrierCount_.compare_exchange_strong(curCount, curCount + delta);
    } while (!done);
}
}  // namespace panda::ecmascript