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

#include "ecmascript/jit/jit_thread.h"
#include "ecmascript/sustaining_js_handle.h"
#include "ecmascript/js_handle.h"

namespace panda::ecmascript {
JitThread::JitThread(JitVM *jitVM) : JSThread(jitVM, ThreadType::JIT_THREAD) {};

JSThread *JitThread::GetHostThread() const
{
    return hostThread_;
}

void JitThread::SetHostThread(JSThread *thread)
{
    hostThread_ = thread;
}

JitVM *JitThread::GetJitVM() const
{
    return static_cast<JitVM*>(JSThread::GetEcmaVM());
}

JSHandle<JSTaggedValue> JitThread::NewHandle(JSTaggedValue value) const
{
    return SustainingJSHandle::NewHandle<JSTaggedValue>(this, value.GetRawData());
}

JitVM *JitVM::Create()
{
    auto vm = new JitVM();
    JitThread *thread = new JitThread(vm);
    vm->jitThread_ = thread;

    // super ecmavm
    vm->InitializeForJit(thread);
    return vm;
}

JitVM::~JitVM()
{
}

void JitVM::Destroy(EcmaVM *compilerVm)
{
    JitVM *jitVM = static_cast<JitVM*>(compilerVm);
    delete jitVM->jitThread_;
    jitVM->jitThread_ = nullptr;

    delete jitVM;
}

void JitVM::SetHostVM(JSThread *hostThread)
{
    hostVm_ = hostThread->GetEcmaVM();
    jitThread_->SetHostThread(hostThread);

    const GlobalEnvConstants *constants = hostThread->GlobalConstants();
    jitThread_->SetGlobalConstants(constants);
}

void JitVM::ReSetHostVM()
{
    hostVm_ = nullptr;
    jitThread_->SetHostThread(nullptr);
    jitThread_->SetGlobalConstants(nullptr);
}

}  // namespace panda::ecmascript
