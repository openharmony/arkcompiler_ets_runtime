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
    JitTask(EcmaVM *vm, Jit *jit, JSHandle<JSFunction> &jsFunction, CString &methodName, int32_t offset,
            uint32_t taskThreadId)
        : vm_(vm),
        jit_(jit),
        jsFunction_(jsFunction),
        compilerTask_(nullptr),
        state_(CompileState::SUCCESS),
        methodInfo_(methodName),
        offset_(offset),
        taskThreadId_(taskThreadId) { }
    ~JitTask();
    void Optimize();
    void Finalize();
    void PrepareCompile();

    void InstallCode();
    void InstallOsrCode(JSHandle<Method> &method, JSHandle<MachineCode> &codeObj);
    MachineCodeDesc *GetMachineCodeDesc()
    {
        return &codeDesc_;
    }

    JSHandle<JSFunction> GetJsFunction() const
    {
        return jsFunction_;
    }

    int32_t GetOffset() const
    {
        return offset_;
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

    bool IsOsrTask()
    {
        return offset_ != MachineCode::INVALID_OSR_OFFSET;
    }

    Jit *GetJit()
    {
        return jit_;
    }

    EcmaVM *GetVM()
    {
        return vm_;
    }

    CString GetMethodInfo() const
    {
        return methodInfo_;
    }

    void SetMethodInfo(CString methodInfo)
    {
        methodInfo_ = methodInfo;
    }

    uint32_t GetTaskThreadId() const
    {
        return taskThreadId_;
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
    void *compilerTask_;
    MachineCodeDesc codeDesc_;
    CompileState state_;
    CString methodInfo_;
    int32_t offset_;
    uint32_t taskThreadId_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_TASK_H
