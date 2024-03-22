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

class ThreadStateTransitionScope final {
public:
    explicit ThreadStateTransitionScope(JSThread* self, ThreadState newState)
        : self_(self)
        {
            ASSERT(self_ != nullptr);
            oldState_ = self_->GetState();
            if (oldState_ != newState) {
                self_->UpdateState(newState);
            } else {
                if (oldState_ == ThreadState::RUNNING) {
                    self_->CheckSafepointIfSuspended();
                }
            }
        }

    ~ThreadStateTransitionScope()
    {
        if (oldState_ != self_->GetState()) {
            self_->UpdateState(oldState_);
        } else {
            if (oldState_ == ThreadState::RUNNING) {
                self_->CheckSafepointIfSuspended();
            }
        }
    }

private:
    JSThread* self_;
    ThreadState oldState_;
    NO_COPY_SEMANTIC(ThreadStateTransitionScope);
};

class ThreadSuspensionScope final {
public:
    explicit ThreadSuspensionScope(JSThread* self) : scope_(self, ThreadState::IS_SUSPENDED)
    {
        ASSERT(self->GetState() == ThreadState::IS_SUSPENDED);
    }

    ~ThreadSuspensionScope() = default;

private:
    ThreadStateTransitionScope scope_;
    NO_COPY_SEMANTIC(ThreadSuspensionScope);
};

class ThreadNativeScope final {
public:
    explicit ThreadNativeScope(JSThread* self) : scope_(self, ThreadState::NATIVE)
    {
        ASSERT(self->GetState() == ThreadState::NATIVE);
    }

    ~ThreadNativeScope() = default;

private:
    ThreadStateTransitionScope scope_;
    NO_COPY_SEMANTIC(ThreadNativeScope);
};

class ThreadManagedScope final {
public:
    explicit ThreadManagedScope(JSThread* self) : scope_(self, ThreadState::RUNNING) {}

    ~ThreadManagedScope() = default;

private:
    ThreadStateTransitionScope scope_;
    NO_COPY_SEMANTIC(ThreadManagedScope);
};

class SuspendAllScope final {
public:
    explicit SuspendAllScope(JSThread* self)
        : self_(self), scope_(self, ThreadState::IS_SUSPENDED)
    {
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SuspendAll");
        Runtime::GetInstance()->SuspendAll(self_);
    }
    ~SuspendAllScope()
    {
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "ResumeAll");
        Runtime::GetInstance()->ResumeAll(self_);
    }
private:
    JSThread* self_;
    ThreadStateTransitionScope scope_;
    NO_COPY_SEMANTIC(SuspendAllScope);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CHECKPOINT_THREAD_STATE_TRANSITION_H
