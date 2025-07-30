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
            if (LIKELY(!self_->IsEnableCMCGC())) {
                if constexpr (std::is_same_v<DaemonThread, T>) {
                    oldState_ = self_->GetState();
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
                    if constexpr (newState == ThreadState::RUNNING) {
                        oldState_ = self_->TransferToRunningIfNonRunning();
                    } else {
                        oldState_ = self_->TransferToNonRunning(newState);
                    }
                    ASSERT(hasSwitchState_ == false);
                    hasSwitchState_ = oldState_ != newState;
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
                    if (hasSwitchState_) {
                        if (isCollectingStats) {
                            vm->IncreaseUpdateThreadStateTransCount();
                        }
                    }
#endif
                }
            } else {
                isEnbaleCMCGC_ = true;
                if constexpr (newState == ThreadState::RUNNING) {
                    hasSwitchState_ = self_->GetThreadHolder()->TransferToRunningIfInNative();
                } else {
                    hasSwitchState_ = self_->GetThreadHolder()->TransferToNativeIfInRunning();
                }
            }
        }

    ~ThreadStateTransitionScope()
    {
        if (LIKELY(hasSwitchState_)) {
            if (LIKELY(!isEnbaleCMCGC_)) {
                if constexpr (std::is_same_v<DaemonThread, T>) {
                    if (oldState_ == ThreadState::RUNNING) {
                        self_->TransferDaemonThreadToRunning();
                    } else {
                        self_->UpdateState(oldState_);
                    }
                } else {
                    if constexpr (newState == ThreadState::RUNNING) {
                        self_->TransferToNonRunningInRunning(oldState_);
                    } else {
                        self_->TransferInNonRunning(oldState_);
                    }
                }
            } else {
                if constexpr (newState == ThreadState::RUNNING) {
                    self_->GetThreadHolder()->TransferToNative();
                } else {
                    self_->GetThreadHolder()->TransferToRunning();
                }
            }
        }
    }

private:
    T* self_;
    ThreadState oldState_;
    uint32_t isEnbaleCMCGC_ {0};
    uint32_t hasSwitchState_ {0};
    NO_COPY_SEMANTIC(ThreadStateTransitionScope);
};

class ThreadSuspensionScope final {
public:
    explicit ThreadSuspensionScope(JSThread* self) : scope_(self)
    {
        ASSERT(!g_isEnableCMCGC || !self->IsInRunningState());
        ASSERT(g_isEnableCMCGC || self->GetState() == ThreadState::IS_SUSPENDED);
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
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SuspendAll", "");
        Runtime::GetInstance()->SuspendAll(self_);
    }
    ~SuspendAllScope()
    {
        TRACE_GC(GCStats::Scope::ScopeId::ResumeAll, SharedHeap::GetInstance()->GetEcmaGCStats());
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ResumeAll", "");
        Runtime::GetInstance()->ResumeAll(self_);
    }
private:
    T* self_;
    ThreadStateTransitionScope<T, ThreadState::IS_SUSPENDED> scope_;
    NO_COPY_SEMANTIC(SuspendAllScope);
};

template<typename T>
class SuspendOtherScope final {
    static_assert(std::is_base_of_v<JSThread, T>);
    static_assert(!std::is_same_v<JitThread, T>);
public:
    explicit SuspendOtherScope(T* self, T* target)
        : self_(self), target_(target), scope_(self)
    {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SuspendOther", "");
        Runtime::GetInstance()->SuspendOther(self_, target_);
    }
    ~SuspendOtherScope()
    {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ResumeOther", "");
        Runtime::GetInstance()->ResumeOther(self_, target_);
    }
private:
    T* self_;
    T* target_;
    ThreadStateTransitionScope<T, ThreadState::IS_SUSPENDED> scope_;
    NO_COPY_SEMANTIC(SuspendOtherScope);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CHECKPOINT_THREAD_STATE_TRANSITION_H
