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

#ifndef ECMASCRIPT_COMPILER_JIT_COMPILER_H
#define ECMASCRIPT_COMPILER_JIT_COMPILER_H

#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/compiler/baseline/baseline_compiler.h"

namespace panda::ecmascript::kungfu {
extern "C" {
PUBLIC_API void InitJitCompiler(JSRuntimeOptions options);
PUBLIC_API void *CreateJitCompilerTask(JitTask *jitTask);
PUBLIC_API bool JitCompile(void *compiler, JitTask *jitTask);
PUBLIC_API bool JitFinalize(void *compiler, JitTask *jitTask);
PUBLIC_API void DeleteJitCompile(void *handle);
};

struct JitCompilationOptions {
    JitCompilationOptions(JSRuntimeOptions options);
    JitCompilationOptions() = default;

    std::string triple_;
    std::string outputFileName_;
    size_t optLevel_;
    size_t relocMode_;
    std::string logOption_;
    std::string logMethodsList_;
    bool compilerLogTime_;
    bool deviceIsScreenOff_;
    uint32_t hotnessThreshold_;
    int32_t deviceThermalLevel_;
    std::string profilerIn_;
    bool isEnableArrayBoundsCheckElimination_;
    bool isEnableTypeLowering_;
    bool isEnableEarlyElimination_;
    bool isEnableLaterElimination_;
    bool isEnableValueNumbering_;
    bool isEnableOptInlining_;
    bool isEnableOptString_;
    bool isEnableOptPGOType_;
    bool isEnableOptTrackField_;
    bool isEnableOptLoopPeeling_;
    bool isEnableOptOnHeapCheck_;
    bool isEnableOptLoopInvariantCodeMotion_;
    bool isEnableOptConstantFolding_;
    bool isEnableLexenvSpecialization_;
    bool isEnableNativeInline_;
    bool isEnableLoweringBuiltin_;
};

class JitCompilerTask final {
public:
    JitCompilerTask(JitTask *jitTask) : jsFunction_(jitTask->GetJsFunction()), offset_(jitTask->GetOffset()),
        jitCompilationEnv_(new JitCompilationEnv(jitTask->GetCompilerVM(), jitTask->GetHostVM(), jsFunction_)),
        profileTypeInfo_(jitTask->GetProfileTypeInfo()),
        compilerTier_(jitTask->GetCompilerTier()), baselineCompiler_(nullptr),
        passManager_(nullptr), jitCodeGenerator_(nullptr) { };
    static JitCompilerTask *CreateJitCompilerTask(JitTask *jitTask);

    bool Compile();
    bool Finalize(JitTask *jitTask);

    void ReleaseJitPassManager();

private:
    JSHandle<JSFunction> jsFunction_;
    int32_t offset_;
    std::unique_ptr<JitCompilationEnv> jitCompilationEnv_;
    JSHandle<ProfileTypeInfo> profileTypeInfo_;
    CompilerTier compilerTier_;
    std::unique_ptr<BaselineCompiler> baselineCompiler_;
    std::unique_ptr<JitPassManager> passManager_;
    // need refact AOTFileGenerator to JitCodeGenerator
    std::unique_ptr<AOTFileGenerator> jitCodeGenerator_;
};

class JitCompiler final {
public:
    explicit JitCompiler(JSRuntimeOptions *options) : jitOptions_(*options),
        log_(jitOptions_.logOption_),
        logList_(jitOptions_.logMethodsList_),
        profilerDecoder_(jitOptions_.profilerIn_, jitOptions_.hotnessThreshold_) { }
    ~JitCompiler() = default;
    void Init(JSRuntimeOptions options);

    static JitCompiler *GetInstance(JSRuntimeOptions *options = nullptr);
    JitCompilationOptions &GetJitOptions()
    {
        return jitOptions_;
    }

    CompilerLog &GetCompilerLog()
    {
        return log_;
    }

    AotMethodLogList &GetLogList()
    {
        return logList_;
    }

    PGOProfilerDecoder &GetProfilerDecoder()
    {
        return profilerDecoder_;
    }
    PassOptions &GetPassOptions()
    {
        return passOptions_;
    }
    void UpdatePassOptions(CompilationEnv *env);

    static ARK_INLINE bool AllocFromFortAndCopy(CompilationEnv &compilationEnv, MachineCodeDesc &desc);
private:
    JitCompilationOptions jitOptions_;
    CompilerLog log_;
    AotMethodLogList logList_;
    PGOProfilerDecoder profilerDecoder_;
    PassOptions passOptions_;
};

}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_JIT_COMPILER_H
