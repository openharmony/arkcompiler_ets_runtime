/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/vm_thread_control.h"

namespace panda::ecmascript {
constexpr int32_t TIME_OUT_MS = 1500;

bool VmThreadControl::NotifyVMThreadSuspension() // block caller thread
{
    if (VMNeedSuspension()) { // only enable one thread to post suspension
        return false;
    }
    SetVMNeedSuspension(true);
    thread_->SetCheckSafePointStatus();
    LockHolder lock(vmThreadSuspensionMutex_);
    while (!IsSuspended()) {
        if (vmThreadNeedSuspensionCV_.TimedWait(&vmThreadSuspensionMutex_, TIME_OUT_MS)) {
            SetVMNeedSuspension(false);
            thread_->ResetCheckSafePointStatus();
            return false;
        }
    }
    return true;
}

void VmThreadControl::RequestTerminateExecution()
{
    SetTerminationRequest(true);
    thread_->SetCheckSafePointStatus();
}

void VmThreadControl::SetVMNeedSuspension(bool flag)
{
    thread_->SetVMNeedSuspension(flag);
}

bool VmThreadControl::VMNeedSuspension() const
{
    return thread_->VMNeedSuspension();
}

void VmThreadControl::SetTerminationRequest(bool flag)
{
    thread_->SetTerminationRequest(flag);
}

void VmThreadControl::SetVMSuspended(bool flag)
{
    thread_->SetVMSuspended(flag);
}

bool VmThreadControl::IsSuspended() const
{
    return thread_->IsVMSuspended();
}

void VmThreadControl::SuspendVM() // block vm thread
{
    LockHolder lock(vmThreadSuspensionMutex_);
    SetVMSuspended(true);
    vmThreadNeedSuspensionCV_.Signal(); // wake up the thread who needs suspend vmthread
    while (VMNeedSuspension()) {
        vmThreadHasSuspendedCV_.Wait(&vmThreadSuspensionMutex_);
    }
    SetVMSuspended(false);
}

void VmThreadControl::ResumeVM()
{
    LockHolder lock(vmThreadSuspensionMutex_);
    SetVMNeedSuspension(false);
    vmThreadHasSuspendedCV_.Signal();
}
}  // namespace panda::ecmascript