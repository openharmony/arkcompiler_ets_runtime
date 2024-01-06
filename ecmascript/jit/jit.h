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
#include "ecmascript/platform/mutex.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/compiler/compiler_log.h"

namespace panda::ecmascript {
class JitTask;
enum JitCompileMode {
    SYNC = 0,
    ASYNC
};
class Jit {
public:
    Jit(EcmaVM *vm) : vm_(vm), initialized_(false), jitCompile_(nullptr), jitFinalize_(nullptr),
        createJitCompiler_(nullptr), deleteJitCompile_(nullptr), libHandle_(nullptr) {};
    ~Jit();
    void Initialize();

    static void Compile(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, JitCompileMode mode = SYNC);
    bool JitCompile(void *compiler, JitTask *jitTask);
    bool JitFinalize(void *compiler, JitTask *jitTask);
    bool IsInitialized() const
    {
        return initialized_;
    }

    void DeleteJitCompile(void *compiler);

    void RequestInstallCode(JitTask *jitTask);
    void InstallTasks();
    void InstallTasksWithoutClearFlag();
    bool IsCompiling(JSHandle<JSFunction> &jsFunction);
    void AddCompilingTask(JitTask *jitTask);
    void RemoveCompilingTask(JitTask *jitTask);

    JitTask *GetAsyncCompileTask();
    void AddAsyncCompileTask(JitTask *jitTask);
    void RemoveAsyncCompileTask(JitTask *jitTask);

    NO_COPY_SEMANTIC(Jit);
    NO_MOVE_SEMANTIC(Jit);

    class Scope : public ClockScope {
    public:
        Scope(CString message) : message_(message) {}
        ~Scope();
    private:
        CString message_;
    };

private:
    bool SupportJIT(Method *method);
    EcmaVM *vm_;
    bool initialized_;
    bool(*jitCompile_)(void*, JitTask*);
    bool(*jitFinalize_)(void*, JitTask*);
    void*(*createJitCompiler_)(EcmaVM*, JitTask*);
    void(*deleteJitCompile_)(void*);
    void *libHandle_;

    std::deque<JitTask*> compilingJitTasks_;
    std::deque<JitTask*> installJitTasks_;
    static std::deque<JitTask*> asyncCompileJitTasks_;
    Mutex installJitTasksDequeMtx_;
    static Mutex asyncCompileJitTasksMtx_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_H
