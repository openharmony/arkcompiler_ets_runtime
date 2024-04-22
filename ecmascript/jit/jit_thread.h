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

#ifndef ECMASCRIPT_JIT_JIT_THREAD_H
#define ECMASCRIPT_JIT_JIT_THREAD_H

#include "ecmascript/js_thread.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
class JitVM;
class JitThread final : public JSThread {
public:
    JitThread(JitVM *jitVM);
    EcmaVM *GetEcmaVM() const;
    JSThread * PUBLIC_API GetHostThread() const;
    void SetHostThread(JSThread *thread);
    JitVM *GetJitVM() const;

private:
    JSThread *hostThread_ {nullptr};
};

class JitVM final : public EcmaVM {
public:
    JitVM() : EcmaVM() {}
    ~JitVM();
    static JitVM *Create();
    static void Destroy(EcmaVM *jitvm);

    EcmaVM *GetHostVm() const
    {
        return hostVm_;
    }

    void SetHostVM(JSThread *hostThread);
    void ReSetHostVM();

private:
    EcmaVM *hostVm_ {nullptr};
    JitThread *jitThread_ {nullptr};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_TASK_H
