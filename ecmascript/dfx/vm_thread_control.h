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

#ifndef ECMASCRIPT_VM_THREAD_CONTROL_H
#define ECMASCRIPT_VM_THREAD_CONTROL_H

#include "ecmascript/js_thread.h"
#include "ecmascript/platform/mutex.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {
class VmThreadControl {
public:
    static constexpr char VM_NEED_SUSPENSION = 1;

    VmThreadControl(JSThread *thread)
    {
        thread_ = thread;
    }

    void SetVMNeedSuspension(bool flag);

    void SetTerminationRequest(bool flag);

    bool VMNeedSuspension() const;

    void SuspendVM();

    void ResumeVM();

    bool NotifyVMThreadSuspension();

    void RequestTerminateExecution();

    void SetVMSuspended(bool flag);

    bool IsSuspended() const;

private:
    JSThread *thread_;
    Mutex vmThreadSuspensionMutex_;
    ConditionVariable vmThreadNeedSuspensionCV_;
    ConditionVariable vmThreadHasSuspendedCV_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_VM_THREAD_CONTROL_H