/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_COMPILER_H
#define ECMASCRIPT_ARKSTEED_COMPILER_H

#include "ecmascript/arksteed/arksteed_task.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"

namespace panda::ecmascript::arksteed {

class ArkSteedAssembler;
class ArkSteedSafepointTableBuilder;
class Graph;

extern "C" {
PUBLIC_API void InitArkSteedCompiler(JSRuntimeOptions options);
PUBLIC_API void *CreateArkSteedCompilerTask(ArkSteedTask *arkSteedTask);
PUBLIC_API bool ArkSteedCompile(void *compiler, ArkSteedTask *arkSteedTask);
PUBLIC_API bool ArkSteedFinalize(void *compiler, ArkSteedTask *arkSteedTask);
PUBLIC_API void DeleteArkSteedCompilerTask(void *handle);
}

struct ArkSteedCompilationOptions {
    ArkSteedCompilationOptions(JSRuntimeOptions options);
    ArkSteedCompilationOptions() = default;

    std::string triple;
    std::string outputFileName;
    bool deviceIsScreenOff;
    uint32_t hotnessThreshold;
    int32_t deviceThermalLevel;
    std::string profilerIn;
    bool enableLog;
    bool printMethodName;
    bool enableCodeComment;
};

class ArkSteedCompilerTask final {
public:
    ArkSteedCompilerTask(ArkSteedTask *arkSteedTask)
        : arkSteedTask_(arkSteedTask),
          jsFunction_(arkSteedTask->GetJsFunction()),
          offset_(arkSteedTask->GetOffset()),
          jitCompilationEnv_(new JitCompilationEnv(arkSteedTask->GetCompilerVM(), arkSteedTask->GetHostVM(),
                                                   jsFunction_, arkSteedTask->GetDependencies())),
          profileTypeInfo_(arkSteedTask->GetProfileTypeInfo()),
          compilerTier_(arkSteedTask->GetCompilerTier())
    {}
    ~ArkSteedCompilerTask();
    static ArkSteedCompilerTask *CreateJitCompilerTask(ArkSteedTask *arkSteedTask);

    bool Compile();

    bool BuildGraph(JSThread *compilerThread, uintptr_t hostGlueAddr);
    void RunPreRegallocProcessors();
    void FillCodeDesc(MachineCodeDesc &codeDesc);

#ifdef JIT_ENABLE_CODE_SIGN
    void EnableCodeSign();
#endif

    JitCompilationEnv *GetCompilationEnv() const
    {
        return jitCompilationEnv_.get();
    }

    FuncEntryDes *GetFuncEntryDes() const
    {
        return funcEntryDes_;
    }

private:
    ArkSteedTask *arkSteedTask_;
    JSHandle<JSFunction> jsFunction_;
    int32_t offset_;
    std::unique_ptr<JitCompilationEnv> jitCompilationEnv_;
    JSHandle<ProfileTypeInfo> profileTypeInfo_;
    CompilerTier compilerTier_;

    // Compilation artifacts — created in Compile(), consumed in FillCodeDesc()
    std::unique_ptr<Chunk> chunk_;
    ArkSteedAssembler *assembler_ = nullptr;
    Graph *graph_ = nullptr;
    ArkSteedSafepointTableBuilder *safepointTableBuilder_ = nullptr;

    // FuncEntryDes - allocated from chunk, written in FillCodeDesc
    FuncEntryDes *funcEntryDes_ = nullptr;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_COMPILER_H
