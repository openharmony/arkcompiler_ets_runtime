/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/jit/jit_thread.h"
#include "ecmascript/sustaining_js_handle.h"

namespace panda::ecmascript {
enum CompileState : uint8_t {
    SUCCESS = 0,
    FAIL,
};

enum RunState : uint8_t {
    INIT = 0,
    RUNNING,
    FINISH
};

class JitTaskpool : public Taskpool {
public:
    PUBLIC_API static JitTaskpool *GetCurrentTaskpool();
    JitTaskpool() = default;
    NO_COPY_SEMANTIC(JitTaskpool);
    NO_MOVE_SEMANTIC(JitTaskpool);

    EcmaVM *GetCompilerVm()
    {
        return compilerVm_;
    }

    void SetCompilerVm(EcmaVM *vm)
    {
        LockHolder lock(jitTaskPoolMutex_);
        compilerVm_ = vm;
        threadId_ = static_cast<int32_t>(compilerVm_->GetJSThread()->GetThreadId());
        jitTaskPoolCV_.SignalAll();
    }

    void WaitForJitTaskPoolReady()
    {
        LockHolder lock(jitTaskPoolMutex_);
        if (compilerVm_ == nullptr) {
            jitTaskPoolCV_.Wait(&jitTaskPoolMutex_);
        }
    }

    void Initialize(bool needInitJitFort)
    {
        Taskpool::Initialize(0, [needInitJitFort](os::thread::native_handle_type thread) {
            os::thread::SetThreadName(thread, "OS_JIT_Thread");
            constexpr int32_t priorityVal = 5; // 5: The priority can be set within range [-20, 19]
            os::thread::SetPriority(os::thread::GetCurrentThreadId(), priorityVal);
            auto jitVm = JitVM::Create();
            JitTaskpool::GetCurrentTaskpool()->SetCompilerVm(jitVm);
            if (needInitJitFort) {
                JitFort::InitJitFortResource();
            }
        }, []([[maybe_unused]] os::thread::native_handle_type thread) {
            EcmaVM *compilerVm = JitTaskpool::GetCurrentTaskpool()->GetCompilerVm();
            JitVM::Destroy(compilerVm);
        });
    }

    void Destroy()
    {
        WaitForJitTaskPoolReady();
        Taskpool::Destroy(threadId_);
    }

private:
    uint32_t TheMostSuitableThreadNum(uint32_t threadNum) const override;
    EcmaVM *compilerVm_ { nullptr };
    Mutex jitTaskPoolMutex_;
    ConditionVariable jitTaskPoolCV_;
    int32_t threadId_ { -1 };
};

class JitTask {
public:
    JitTask(JSThread *hostThread, JSThread *compilerThread, Jit *jit,
        JSHandle<JSFunction> &jsFunction, CompilerTier tier, CString &methodName, int32_t offset,
        JitCompileMode mode);
    // for ut
    JitTask(EcmaVM *hVm, EcmaVM *cVm, Jit *jit, JitCompileMode mode);
    ~JitTask();
    void Optimize();
    void Finalize();
    void PrepareCompile();

    void InstallCode();
    void InstallOsrCode(JSHandle<MachineCode> &codeObj);
    void InstallCodeByCompilerTier(JSHandle<MachineCode> &machineCode,
        JSHandle<Method> &methodHandle);
    MachineCodeDesc &GetMachineCodeDesc()
    {
        return codeDesc_;
    }

    RelocMap &GetRelocInfo()
    {
        return relocInfo_;
    }

    JSHandle<JSFunction> GetJsFunction() const
    {
        return jsFunction_;
    }

    int32_t GetOffset() const
    {
        return offset_;
    }

    JSHandle<ProfileTypeInfo> GetProfileTypeInfo() const
    {
        return profileTypeInfo_;
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

    CompilerTier GetCompilerTier() const
    {
        return compilerTier_;
    }

    Jit *GetJit()
    {
        return jit_;
    }

    JSThread *GetHostThread()
    {
        return hostThread_;
    }

    EcmaVM *GetHostVM()
    {
        return hostThread_->GetEcmaVM();
    }

    JSThread *GetCompilerThread()
    {
        return compilerThread_;
    }

    JitVM *GetCompilerVM()
    {
        return static_cast<JitVM*>(compilerThread_->GetEcmaVM());
    }

    CString GetMethodName() const
    {
        return methodName_;
    }

    void SetMethodInfo(CString methodName)
    {
        methodName_ = methodName;
    }

    EcmaContext *GetEcmaContext() const
    {
        return ecmaContext_;
    }

    void SetRunState(RunState s)
    {
        runState_.store(s, std::memory_order_release);
    }

    void SetRunStateFinish()
    {
        LockHolder lock(runStateMutex_);
        runState_.store(RunState::FINISH);
        runStateCondition_.SignalAll();
    }

    bool IsFinish() const
    {
        return runState_.load(std::memory_order_acquire) == RunState::FINISH;
    }
    bool IsRunning() const
    {
        return runState_.load(std::memory_order_acquire) == RunState::RUNNING;
    }
    void WaitFinish();
    bool IsAsyncTask() const
    {
        return jitCompileMode_.IsAsync();
    }

    void SetMainThreadCompilerTime(int time)
    {
        mainThreadCompileTime_ = time;
    }

    int GetMainThreadCompilerTime() const
    {
        return mainThreadCompileTime_;
    }
    static size_t PUBLIC_API ComputePayLoadSize(MachineCodeDesc &codeDesc);

    class AsyncTask : public Task {
    public:
        explicit AsyncTask(std::shared_ptr<JitTask>jitTask, int32_t id) : Task(id), jitTask_(jitTask) { }
        virtual ~AsyncTask() override = default;

        bool Run(uint32_t threadIndex) override;
        EcmaContext *GetEcmaContext() const
        {
            return jitTask_->GetEcmaContext();
        }
        EcmaVM *GetHostVM() const
        {
            return jitTask_->GetHostThread()->GetEcmaVM();
        }
        bool IsRunning() const
        {
            return jitTask_->IsRunning();
        }
        void WaitFinish() const
        {
            jitTask_->WaitFinish();
        }

        void Terminated()
        {
            Task::Terminated();
        }

        void ReleaseSustainingJSHandle()
        {
            jitTask_->ReleaseSustainingJSHandle();
        }
    private:
        std::shared_ptr<JitTask> jitTask_ { nullptr };

        class AsyncTaskRunScope {
        public:
            AsyncTaskRunScope(JitTask *jitTask);
            ~AsyncTaskRunScope();
        private:
            JitTask *jitTask_ { nullptr };
            JitVM *jitvm_ { nullptr };
        };
    };
private:
    void SustainingJSHandles();
    void ReleaseSustainingJSHandle();
    void CloneProfileTypeInfo();
    void SetJsFunction(JSHandle<JSFunction> &jsFunction)
    {
        jsFunction_ = jsFunction;
    }

    void SetProfileTypeInfo(JSHandle<ProfileTypeInfo> &profileTypeInfo)
    {
        profileTypeInfo_ = profileTypeInfo;
    }

    JSThread *hostThread_;
    JSThread *compilerThread_;
    Jit *jit_;
    JSHandle<JSFunction> jsFunction_;
    JSHandle<ProfileTypeInfo> profileTypeInfo_;
    void *compilerTask_;
    MachineCodeDesc codeDesc_;
    CompileState state_;
    CompilerTier compilerTier_;
    CString methodName_;
    int32_t offset_;
    std::unique_ptr<SustainingJSHandle> sustainingJSHandle_;
    EcmaContext *ecmaContext_;
    JitCompileMode jitCompileMode_;
    JitDfx *jitDfx_ { nullptr };
    int mainThreadCompileTime_ {0};

    std::atomic<RunState> runState_;
    Mutex runStateMutex_;
    ConditionVariable runStateCondition_;
    RelocMap relocInfo_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_TASK_H
