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

#ifndef ECMASCRIPT_JIT_H
#define ECMASCRIPT_JIT_H

#include "ecmascript/common.h"
#include "ecmascript/compiler/compilation_env.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/jit/jit_thread.h"
#include "ecmascript/jit/jit_dfx.h"

namespace panda::ecmascript {
class JitTask;
enum JitCompileMode {
    SYNC = 0,
    ASYNC
};

enum class CompilerTier : uint8_t {
    BASELINE,
    FAST,
};

class Jit {
public:
    Jit() {}
    ~Jit();
    static PUBLIC_API Jit *GetInstance();
    void SetEnableOrDisable(const JSRuntimeOptions &options, bool isEnableFastJit, bool isEnableBaselineJit);
    bool PUBLIC_API IsEnableFastJit() const;
    bool PUBLIC_API IsEnableBaselineJit() const;
    void Initialize();

    static void Compile(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, CompilerTier tier = CompilerTier::FAST,
                        int32_t offset = MachineCode::INVALID_OSR_OFFSET, JitCompileMode mode = SYNC);
    bool JitCompile(void *compiler, JitTask *jitTask);
    bool JitFinalize(void *compiler, JitTask *jitTask);
    void *CreateJitCompilerTask(JitTask *jitTask);
    bool IsInitialized() const
    {
        return initialized_;
    }

    void DeleteJitCompile(void *compiler);

    void RequestInstallCode(std::shared_ptr<JitTask> jitTask);
    void InstallTasks(uint32_t threadId);
    void ClearTask(const std::function<bool(Task *task)> &checkClear);
    void ClearTask(EcmaContext *ecmaContext);
    void ClearTaskWithVm(EcmaVM *vm);
    void Destroy();
    void CheckMechineCodeSpaceMemory(JSThread *thread, int remainSize);
    void ChangeTaskPoolState(bool inBackground);

    // dfx for jit warmup compile
    static void CountInterpExecFuncs(JSHandle<JSFunction> &jsFunction);

    bool ReuseCompiledFunc(JSThread *thread, JSHandle<JSFunction> &function);

    bool IsAppJit() const
    {
        return isApp_;
    }

    void SetProfileNeedDump(bool isNeed)
    {
        isProfileNeedDump_ = isNeed;
    }

    bool IsProfileNeedDump() const
    {
        return isProfileNeedDump_;
    }

    JitDfx *GetJitDfx() const
    {
        return jitDfx_;
    }
    NO_COPY_SEMANTIC(Jit);
    NO_MOVE_SEMANTIC(Jit);

    class TimeScope : public ClockScope {
    public:
        explicit TimeScope(CString message, CompilerTier tier = CompilerTier::FAST, bool outPutLog = true,
            bool isDebugLevel = false)
            : message_(message), tier_(tier), outPutLog_(outPutLog), isDebugLevel_(isDebugLevel) {}
        explicit TimeScope() : message_(""), tier_(CompilerTier::FAST), outPutLog_(false), isDebugLevel_(true) {}
        PUBLIC_API ~TimeScope();
    private:
        CString message_;
        CompilerTier tier_;
        bool outPutLog_;
        bool isDebugLevel_;
    };

    class JitLockHolder {
    public:
        explicit JitLockHolder(const CompilationEnv *env) : thread_(nullptr), scope_()
        {
            if (env->IsJitCompiler()) {
                JSThread *thread = env->GetJSThread();
                ASSERT(thread->IsJitThread());
                thread_ = static_cast<JitThread*>(thread);
                if (thread_->GetState() != ThreadState::RUNNING) {
                    thread_->ManagedCodeBegin();
                    isInManagedCode_ = true;
                }
                thread_->GetHostThread()->GetJitLock()->Lock();
            }
        }

        explicit JitLockHolder(const CompilationEnv *env, CString message) : thread_(nullptr),
            scope_("Jit Compile Pass: " + message + ", Time:", CompilerTier::FAST, false)
        {
            if (env->IsJitCompiler()) {
                JSThread *thread = env->GetJSThread();
                ASSERT(thread->IsJitThread());
                thread_ = static_cast<JitThread*>(thread);
                if (thread_->GetState() != ThreadState::RUNNING) {
                    thread_->ManagedCodeBegin();
                    isInManagedCode_ = true;
                }
                thread_->GetHostThread()->GetJitLock()->Lock();
            }
        }

        ~JitLockHolder()
        {
            if (thread_ != nullptr) {
                thread_->GetHostThread()->GetJitLock()->Unlock();
                if (isInManagedCode_) {
                    thread_->ManagedCodeEnd();
                }
            }
        }
        JitThread *thread_ {nullptr};
        TimeScope scope_;
        bool isInManagedCode_{false};
        ALLOW_HEAP_ACCESS
        NO_COPY_SEMANTIC(JitLockHolder);
        NO_MOVE_SEMANTIC(JitLockHolder);
    };

    class JitGCLockHolder {
    public:
        explicit JitGCLockHolder(JSThread *thread) : thread_(thread)
        {
            ASSERT(!thread->IsJitThread());
            if (Jit::GetInstance()->IsEnableFastJit() || Jit::GetInstance()->IsEnableBaselineJit()) {
                Clock::time_point start = Clock::now();
                thread_->GetJitLock()->Lock();
                Jit::GetInstance()->GetJitDfx()->SetLockHoldingTime(
                    std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count());
                locked_ = true;
            }
        }

        ~JitGCLockHolder()
        {
            if (locked_) {
                thread_->GetJitLock()->Unlock();
                locked_ = false;
            }
        }

    private:
        JSThread *thread_;
        bool locked_ { false };

        NO_COPY_SEMANTIC(JitGCLockHolder);
        NO_MOVE_SEMANTIC(JitGCLockHolder);
    };

private:
    bool SupportJIT(const Method *method) const;
    bool initialized_ { false };
    bool fastJitEnable_ { false };
    bool baselineJitEnable_ { false };
    bool isApp_ { false };
    bool isProfileNeedDump_ { true };

    std::unordered_map<uint32_t, std::deque<std::shared_ptr<JitTask>>> installJitTasks_;
    Mutex installJitTasksDequeMtx_;
    Mutex setEnableLock_;

    JitDfx *jitDfx_ { nullptr };
    static constexpr int MIN_CODE_SPACE_SIZE = 1_KB;

    static void (*initJitCompiler_)(JSRuntimeOptions);
    static bool(*jitCompile_)(void*, JitTask*);
    static bool(*jitFinalize_)(void*, JitTask*);
    static void*(*createJitCompilerTask_)(JitTask*);
    static void(*deleteJitCompile_)(void*);
    static void *libHandle_;
    static bool CheckJitCompileStatus(JSHandle<JSFunction> &jsFunction,
        const CString &methodName, CompilerTier tier);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_H
