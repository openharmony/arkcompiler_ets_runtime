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

class ThreadSuspensionScope final {
public:
    inline explicit ThreadSuspensionScope(JSThread* self) : self_(self)
    {
        oldState_ = self_->GetState();
        self_->UpdateState(ThreadState::IS_SUSPENDED);
    }

    inline ~ThreadSuspensionScope()
    {
        self_->UpdateState(oldState_);
    }

private:
    JSThread* self_;
    ThreadState oldState_;
    NO_COPY_SEMANTIC(ThreadSuspensionScope);
};

class ThreadStateTransitionScope final {
public:
    inline ThreadStateTransitionScope(JSThread* self, ThreadState newState)
        : self_(self)
        {
            ASSERT(self_ != nullptr);
            oldState_ = self_->GetState();
            self_->UpdateState(newState);
        }

    inline ~ThreadStateTransitionScope()
    {
        self_->UpdateState(oldState_);
    }

private:
    JSThread* self_;
    ThreadState oldState_;
    NO_COPY_SEMANTIC(ThreadStateTransitionScope);
};

class SuspendAllScope final {
public:
    inline explicit SuspendAllScope(JSThread* self)
        : self_(self), tst_(self, ThreadState::IS_SUSPENDED)
    {
        Runtime::GetInstance()->SuspendAll(self_);
    }
    inline ~SuspendAllScope()
    {
        Runtime::GetInstance()->ResumeAll(self_);
    }
private:
    JSThread* self_;
    ThreadStateTransitionScope tst_;
    NO_COPY_SEMANTIC(SuspendAllScope);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CHECKPOINT_THREAD_STATE_TRANSITION_H
