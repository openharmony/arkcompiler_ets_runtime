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
#if defined(PANDA_USE_FUTEX)
            int32_t *addr = reinterpret_cast<int32_t*>(&passBarrierCount_);
            if (futex(addr, FUTEX_WAIT_PRIVATE, curCount, nullptr, nullptr, 0) != 0) {
                if (errno != EAGAIN && errno != EINTR) {
                    LOG_GC(FATAL) << "SuspendBarrier::Wait failed, errno = " << errno;
                    UNREACHABLE();
                }
            }
#else
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
}  // namespace panda::ecmascript