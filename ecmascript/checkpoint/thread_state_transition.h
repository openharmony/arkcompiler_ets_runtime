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

#ifndef ECMASCRIPT_CHECKPOINT_THREAD_STATE_TRANSITION_H
#define ECMASCRIPT_CHECKPOINT_THREAD_STATE_TRANSITION_H

#include "ecmascript/runtime.h"
#ifdef USE_CMC_GC
#include "ecmascript/crt.h"
#endif

namespace panda::ecmascript {

template<typename T, ThreadState newState>
class ThreadStateTransitionScope final {
    static_assert(std::is_base_of_v<JSThread, T>);
    static_assert(!std::is_same_v<JitThread, T>);
public:
    explicit ThreadStateTransitionScope(T* self)
        : self_(self)
        {
            ASSERT(self_ != nullptr);
#if USE_CMC_GC
            if constexpr (newState == ThreadState::RUNNING) {
                hasSwitchState_ = self_->GetThreadHolder()->TransferToRunningIfInNative();
            } else {
                hasSwitchState_ = self_->GetThreadHolder()->TransferToNativeIfInRunning();
            }
#else
            oldState_ = self_->GetState();
            if constexpr (std::is_same_v<DaemonThread, T>) {
                if (oldState_ != newState) {
                    ASSERT(hasSwitchState_ == false);
                    hasSwitchState_ = true;
                    if constexpr (newState == ThreadState::RUNNING) {
                        self_->TransferDaemonThreadToRunning();
                    } else {
                        self_->UpdateState(newState);
                    }
                }
            } else {
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
                auto vm = self_->GetEcmaVM();
                bool isCollectingStats = vm->IsCollectingScopeLockStats();
                if (isCollectingStats) {
                    vm->IncreaseEnterThreadManagedScopeCount();
                }
#endif
                if (oldState_ != newState) {
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
                    if (isCollectingStats) {
                        vm->IncreaseUpdateThreadStateTransCount();
                    }
#endif
                    ASSERT(hasSwitchState_ == false);
                    hasSwitchState_ = true;
                    self_->UpdateState(newState);
                }
            }
#endif
        }

    ~ThreadStateTransitionScope()
    {
#if USE_CMC_GC
        if (hasSwitchState_) {
            if constexpr (newState == ThreadState::RUNNING) {
                self_->GetThreadHolder()->TransferToNative();
            } else {
                self_->GetThreadHolder()->TransferToRunning();
            }
        }
#else
        if (hasSwitchState_) {
            if constexpr (std::is_same_v<DaemonThread, T>) {
                if (oldState_ == ThreadState::RUNNING) {
                    self_->TransferDaemonThreadToRunning();
                } else {
                    self_->UpdateState(oldState_);
                }
            } else {
                self_->UpdateState(oldState_);
            }
        }
#endif
    }

private:
    T* self_;
    ThreadState oldState_;
    bool hasSwitchState_ {false};
    NO_COPY_SEMANTIC(ThreadStateTransitionScope);
};

class ThreadSuspensionScope final {
public:
    explicit ThreadSuspensionScope(JSThread* self) : scope_(self)
    {
#if USE_CMC_GC
        ASSERT(!self->IsInRunningState());
#else
        ASSERT(self->GetState() == ThreadState::IS_SUSPENDED);
#endif
    }

    ~ThreadSuspensionScope() = default;

private:
    ThreadStateTransitionScope<JSThread, ThreadState::IS_SUSPENDED> scope_;
    NO_COPY_SEMANTIC(ThreadSuspensionScope);
};

class ThreadNativeScope final {
public:
    explicit ThreadNativeScope(JSThread* self) : scope_(self)
    {
        ASSERT(!self->IsInRunningState());
    }

    ~ThreadNativeScope() = default;

private:
    ThreadStateTransitionScope<JSThread, ThreadState::NATIVE> scope_;
    NO_COPY_SEMANTIC(ThreadNativeScope);
};

template<typename T>
class ThreadManagedScope final {
    static_assert(std::is_base_of_v<JSThread, T>);
    static_assert(!std::is_same_v<JitThread, T>);
public:
    explicit ThreadManagedScope(T* self) : scope_(self) {}

    ~ThreadManagedScope() = default;

private:
    ThreadStateTransitionScope<T, ThreadState::RUNNING> scope_;
    NO_COPY_SEMANTIC(ThreadManagedScope);
};

template<typename T>
class SuspendAllScope final {
    static_assert(std::is_base_of_v<JSThread, T>);
    static_assert(!std::is_same_v<JitThread, T>);
public:
    explicit SuspendAllScope(T* self)
        : self_(self), scope_(self)
    {
        TRACE_GC(GCStats::Scope::ScopeId::SuspendAll, SharedHeap::GetInstance()->GetEcmaGCStats());
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SuspendAll");
        Runtime::GetInstance()->SuspendAll(self_);
    }
    ~SuspendAllScope()
    {
        TRACE_GC(GCStats::Scope::ScopeId::ResumeAll, SharedHeap::GetInstance()->GetEcmaGCStats());
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "ResumeAll");
        Runtime::GetInstance()->ResumeAll(self_);
    }
private:
    T* self_;
    ThreadStateTransitionScope<T, ThreadState::IS_SUSPENDED> scope_;
    NO_COPY_SEMANTIC(SuspendAllScope);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CHECKPOINT_THREAD_STATE_TRANSITION_H
