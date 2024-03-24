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
    Jit() {}
    ~Jit();
    static Jit *GetInstance();
    void SetEnableOrDisable(const JSRuntimeOptions &options, bool isEnable);
    bool IsEnable();
    void Initialize();

    static void Compile(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, JitCompileMode mode = SYNC);
    bool JitCompile(void *compiler, JitTask *jitTask);
    bool JitFinalize(void *compiler, JitTask *jitTask);
    void *CreateJitCompilerTask(JitTask *jitTask);
    bool IsInitialized() const
    {
        return initialized_;
    }

    void DeleteJitCompile(void *compiler);

    void RequestInstallCode(JitTask *jitTask);
    void InstallTasks(uint32_t threadId);

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
    bool initialized_ { false };
    bool jitEnable_ { false };

    std::unordered_map<uint32_t, std::deque<JitTask*>> installJitTasks_;
    static std::deque<JitTask*> asyncCompileJitTasks_;
    Mutex installJitTasksDequeMtx_;
    static Mutex asyncCompileJitTasksMtx_;

    static void (*initJitCompiler_)(JSRuntimeOptions);
    static bool(*jitCompile_)(void*, JitTask*);
    static bool(*jitFinalize_)(void*, JitTask*);
    static void*(*createJitCompilerTask_)(JitTask*);
    static void(*deleteJitCompile_)(void*);
    static void *libHandle_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_H
