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

#ifndef ECMASCRIPT_JIT_RESOURCES_H
#define ECMASCRIPT_JIT_RESOURCES_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda::ecmascript {
class JitTask;
using InitJitCompilerFuncType = void (*)(JSRuntimeOptions options);
using JitCompileFuncType = bool(*)(void*, JitTask*);
using JitFinalizeFuncType = bool(*)(void*, JitTask*);
using CreateJitCompilerTaskFuncType = void*(*)(JitTask*);
using DeleteJitCompilerTaskFuncType = void(*)(void*);

class JitResources {
public:
    JitResources() = default;
    ~JitResources() = default;

    void Resolve();
    void Destroy();

    bool IsLibResolved() const
    {
        return libResolved_;
    }

    bool Compile(void *compiler, JitTask *jitTask)
    {
        if (jitCompile_ == nullptr) {
            return false;
        }
        return jitCompile_(compiler, jitTask);
    }

    bool Finalize(void *compiler, JitTask *jitTask)
    {
        if (jitFinalize_ == nullptr) {
            return false;
        }
        return jitFinalize_(compiler, jitTask);
    }

    void *CreateJitCompilerTask(JitTask *jitTask)
    {
        if (createJitCompilerTask_ == nullptr) {
            return nullptr;
        }
        return createJitCompilerTask_(jitTask);
    }

    void DeleteJitCompilerTask(void *compilerTask)
    {
        if (deleteJitCompilerTask_ == nullptr) {
            return;
        }
        deleteJitCompilerTask_(compilerTask);
    }
    bool ResolveLib();
    bool InitJitEnv(const JSRuntimeOptions &options);

private:
    void DoResolve();
    void InitCompiler();
    void InitJitTaskpool();

    JSRuntimeOptions jsRuntimeOptions_;
    bool libResolved_ {false};

    InitJitCompilerFuncType initJitCompiler_ {nullptr};
    JitCompileFuncType jitCompile_ {nullptr};
    JitFinalizeFuncType jitFinalize_ {nullptr};
    CreateJitCompilerTaskFuncType createJitCompilerTask_ {nullptr};
    DeleteJitCompilerTaskFuncType deleteJitCompilerTask_ {nullptr};
    void *libHandle_ {nullptr};

    NO_COPY_SEMANTIC(JitResources);
    NO_MOVE_SEMANTIC(JitResources);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_RESOURCES_H
