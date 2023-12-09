/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JIT_TASK_H
#define ECMASCRIPT_JIT_TASK_H

#include "ecmascript/common.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/ecma_vm.h"

#include "ecmascript/jit/jit.h"

namespace panda::ecmascript {
enum CompileState : uint8_t {
    SUCCESS = 0,
    FAIL,
};

class JitTask {
public:
    JitTask(EcmaVM *vm, Jit *jit, JSHandle<JSFunction> &jsFunction) : vm_(vm), jit_(jit),
        jsFunction_(jsFunction), compiler_(nullptr), state_(CompileState::SUCCESS) {
            PersistentHandle();
        }
    ~JitTask();
    void Optimize();
    void Finalize();

    void InstallCode();
    MachineCodeDesc *GetMachineCodeDesc()
    {
        return &codeDesc_;
    }

    JSHandle<JSFunction> GetJsFunction() const
    {
        return jsFunction_;
    }

    void SetCompiler(void *compiler)
    {
        compiler_ = compiler;
    }

    void RequestInstallCode()
    {
        jit_->RequestInstallCode(this);
    }

    bool IsCompileSuccess() const
    {
        return state_ == CompileState::SUCCESS;
    }

    void SetCompileFailed()
    {
        state_ = CompileState::FAIL;
    }

    class AsyncTask : public Task {
    public:
        explicit AsyncTask(JitTask *jitTask, int32_t id) : Task(id), jitTask_(jitTask) { }
        virtual ~AsyncTask() override = default;

        bool Run(uint32_t threadIndex) override;
    private:
        JitTask *jitTask_;
    };
private:
    void PersistentHandle();
    void ReleasePersistentHandle();
    void SetJsFunction(JSHandle<JSFunction> &jsFunction)
    {
        jsFunction_ = jsFunction;
    }

    EcmaVM *vm_;
    Jit *jit_;
    JSHandle<JSFunction> jsFunction_;
    void *compiler_;
    MachineCodeDesc codeDesc_;
    CompileState state_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_TASK_H
