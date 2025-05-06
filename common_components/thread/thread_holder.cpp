/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "common_interfaces/thread/thread_holder-inl.h"

#include "common_components/common_runtime/src/mutator/mutator.h"
#include "common_interfaces/thread/base_thread.h"
#include "common_interfaces/base_runtime.h"

namespace panda {
thread_local ThreadHolder *currentThreadHolder = nullptr;

extern "C" void VisitJSThread(void *jsThread, CommonRootVisitor visitor);

ThreadHolder *ThreadHolder::CreateAndRegisterNewThreadHolder(void *vm)
{
    if (ThreadLocal::IsArkProcessor() || ThreadLocal::GetMutator() != nullptr) {
        LOG_COMMON(FATAL) << "CreateAndRegisterNewThreadHolder fail";
        return nullptr;
    }
    Mutator* mutator = panda::Mutator::NewMutator();
    CHECK_CC(mutator != nullptr);
    mutator->SetEcmaVMPtr(vm);
    ThreadHolder *holder = mutator->GetThreadHolder();
    BaseRuntime::GetInstance()->GetThreadHolderManager().RegisterThreadHolder(holder);
    return holder;
}

ThreadHolder *ThreadHolder::GetCurrent()
{
    return currentThreadHolder;
}

void ThreadHolder::SetCurrent(ThreadHolder *holder)
{
    currentThreadHolder = holder;
}

void ThreadHolder::RegisterJSThread(JSThread *jsThread)
{
    DCHECK_CC(!IsInRunningState());
    TransferToRunning();
    DCHECK_CC(jsThread_ == nullptr);
    jsThread_ = jsThread;
    TransferToNative();
}

void ThreadHolder::UnregisterJSThread(JSThread *jsThread)
{
    DCHECK_CC(!IsInRunningState());
    TransferToRunning();
    DCHECK_CC(jsThread_ == jsThread);
    jsThread_ = nullptr;
    TransferToNative();
    if (threads_.empty()) {
        BaseRuntime::GetInstance()->GetThreadHolderManager().UnregisterThreadHolder(this);
    }
}

void ThreadHolder::RegisterCoroutine(Coroutine *coroutine)
{
    DCHECK_CC(!IsInRunningState());
    TransferToRunning();
    DCHECK_CC(coroutines_.find(coroutine) == coroutines_.end());
    coroutines_.insert(coroutine);
    TransferToNative();
}

void ThreadHolder::UnregisterCoroutine(Coroutine *coroutine)
{
    DCHECK_CC(!IsInRunningState());
    TransferToRunning();
    DCHECK_CC(coroutines_.find(coroutine) != coroutines_.end());
    coroutines_.erase(coroutine);
    TransferToNative();
    if (threads_.empty() && jsThread_ == nullptr) {
        BaseRuntime::GetInstance()->GetThreadHolderManager().UnregisterThreadHolder(this);
    }
}

void ThreadHolder::WaitSuspension()
{
    mutatorBase_->HandleSuspensionRequest();
}

void ThreadHolder::VisitAllThreads(CommonRootVisitor visitor)
{
    if (jsThread_ != nullptr) {
        VisitJSThread(jsThread_, visitor);
    }
    for (auto *coroutine : coroutines_) {
        // Depending on the integrated so
    }
}
} // namespace panda
