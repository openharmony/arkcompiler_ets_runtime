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

#include <chrono>
#include <iostream>
#include <csignal>  // NOLINTNEXTLINE(modernize-deprecated-headers)
#include <vector>

#include "ecmascript/compiler/jit_compiler.h"
#include "ecmascript/compiler/baseline/baseline_compiler.h"

#include "ecmascript/jit/jit_task.h"
#include "ecmascript/log.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript::kungfu {
JitCompiler *JitCompiler::GetInstance(JSRuntimeOptions *options)
{
    static JitCompiler instance(options);
    return &instance;
}

void JitCompiler::UpdatePassOptions(CompilationEnv *env)
{
    EcmaVM *vm = env->GetHostThread()->GetEcmaVM();
    bool builtinsLazyEnabled = vm->GetJSOptions().IsWorker() && vm->GetJSOptions().GetEnableBuiltinsLazy();
    if (builtinsLazyEnabled) {
        passOptions_.SetLoweringBuiltin(false);
    }
}

JitCompilationOptions::JitCompilationOptions(JSRuntimeOptions runtimeOptions)
{
#if defined(PANDA_TARGET_AMD64)
    triple_ = TARGET_X64;
#elif defined(PANDA_TARGET_ARM64)
    triple_ = TARGET_AARCH64;
#else
    LOG_JIT(FATAL) << "jit unsupport arch";
    UNREACHABLE();
#endif
    // refactor: remove JitCompilationOptions, reuse CompilationOptions
    optLevel_ = runtimeOptions.GetOptLevel();
    relocMode_ = runtimeOptions.GetRelocMode();
    logOption_ = runtimeOptions.GetCompilerLogOption();
    logMethodsList_ = runtimeOptions.GetMethodsListForLog();
    compilerLogTime_ = runtimeOptions.IsEnableCompilerLogTime();
    deviceIsScreenOff_ = runtimeOptions.GetDeviceState();
    deviceThermalLevel_ = runtimeOptions.GetThermalLevel();
    hotnessThreshold_ = runtimeOptions.GetPGOHotnessThreshold();
    profilerIn_ = std::string(runtimeOptions.GetPGOProfilerPath());
    isEnableArrayBoundsCheckElimination_ = runtimeOptions.IsEnableArrayBoundsCheckElimination();
    isEnableTypeLowering_ = (runtimeOptions.IsEnableTypeLowering()) && (!runtimeOptions.IsEnableJitFastCompile());
    isEnableEarlyElimination_ = runtimeOptions.IsEnableEarlyElimination();
    isEnableLaterElimination_ = runtimeOptions.IsEnableLaterElimination();
    isEnableValueNumbering_ = runtimeOptions.IsEnableValueNumbering();
    isEnableOptInlining_ = runtimeOptions.IsEnableAPPJIT() ? false : runtimeOptions.IsEnableOptInlining();
    isEnableOptString_ = runtimeOptions.IsEnableOptString();
    isEnableOptPGOType_ = runtimeOptions.IsEnableOptPGOType();
    isEnableOptTrackField_ = runtimeOptions.IsEnableOptTrackField();
    isEnableOptLoopPeeling_ = runtimeOptions.IsEnableOptLoopPeeling();
    isEnableOptOnHeapCheck_ = runtimeOptions.IsEnableOptOnHeapCheck();
    isEnableOptLoopInvariantCodeMotion_ = runtimeOptions.IsEnableOptLoopInvariantCodeMotion();
    isEnableOptConstantFolding_ = runtimeOptions.IsEnableOptConstantFolding();
    isEnableLexenvSpecialization_ = runtimeOptions.IsEnableLexenvSpecialization();
    isEnableNativeInline_ = runtimeOptions.IsEnableNativeInline();
    isEnableLoweringBuiltin_ = runtimeOptions.IsEnableLoweringBuiltin();
}

void JitCompiler::Init(JSRuntimeOptions runtimeOptions)
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    BuiltinsStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();

    JitCompilationOptions jitOptions(runtimeOptions);
    jitOptions_ = jitOptions;
    PassOptions::Builder optionsBuilder;
    passOptions_ =
        optionsBuilder.EnableArrayBoundsCheckElimination(jitOptions_.isEnableArrayBoundsCheckElimination_)
            .EnableTypeLowering(jitOptions_.isEnableTypeLowering_)
            .EnableEarlyElimination(jitOptions_.isEnableEarlyElimination_)
            .EnableLaterElimination(jitOptions_.isEnableLaterElimination_)
            .EnableValueNumbering(jitOptions_.isEnableValueNumbering_)
            .EnableOptInlining(jitOptions_.isEnableOptInlining_)
            .EnableOptString(jitOptions_.isEnableOptString_)
            .EnableOptPGOType(jitOptions_.isEnableOptPGOType_)
            .EnableOptTrackField(jitOptions_.isEnableOptTrackField_)
            .EnableOptLoopPeeling(jitOptions_.isEnableOptLoopPeeling_)
            .EnableOptLoopInvariantCodeMotion(jitOptions_.isEnableOptLoopInvariantCodeMotion_)
            .EnableOptConstantFolding(jitOptions_.isEnableOptConstantFolding_)
            .EnableLexenvSpecialization(jitOptions_.isEnableLexenvSpecialization_)
            .EnableInlineNative(jitOptions_.isEnableNativeInline_)
            .EnableLoweringBuiltin(jitOptions_.isEnableLoweringBuiltin_)
            .Build();
}

JitCompilerTask *JitCompilerTask::CreateJitCompilerTask(JitTask *jitTask)
{
    return new (std::nothrow) JitCompilerTask(jitTask);
}

bool JitCompilerTask::Compile()
{
    if (compilerTier_ == CompilerTier::BASELINE) {
        auto baselineCompiler =
            new (std::nothrow) BaselineCompiler(jitCompilationEnv_->GetHostThread()->GetEcmaVM());
        if (baselineCompiler == nullptr) {
            return false;
        }
        baselineCompiler_.reset(baselineCompiler);
        baselineCompiler_->Compile(jitCompilationEnv_->GetJSPandaFile(), jitCompilationEnv_->GetMethodLiteral());
        return true;
    }

    JitCompiler *jitCompiler = JitCompiler::GetInstance();
    jitCompiler->UpdatePassOptions(jitCompilationEnv_.get());
    auto jitPassManager = new (std::nothrow) JitPassManager(jitCompilationEnv_.get(),
                                                            jitCompiler->GetJitOptions().triple_,
                                                            jitCompiler->GetJitOptions().optLevel_,
                                                            jitCompiler->GetJitOptions().relocMode_,
                                                            &jitCompiler->GetCompilerLog(),
                                                            &jitCompiler->GetLogList(),
                                                            jitCompiler->GetProfilerDecoder(),
                                                            &jitCompiler->GetPassOptions());
    if (jitPassManager == nullptr) {
        return false;
    }
    passManager_.reset(jitPassManager);
    auto aotFileGenerator = new (std::nothrow) AOTFileGenerator(&jitCompiler->GetCompilerLog(),
        &jitCompiler->GetLogList(), jitCompilationEnv_.get(),
        jitCompiler->GetJitOptions().triple_, jitCompilationEnv_->GetJSOptions().IsCompilerEnableLiteCG());
    if (aotFileGenerator == nullptr) {
        return false;
    }
    jitCodeGenerator_.reset(aotFileGenerator);
    return passManager_->Compile(profileTypeInfo_, *jitCodeGenerator_, offset_);
}

void JitCompilerTask::ReleaseJitPassManager()
{
    // release passManager before jitCompilerTask release,
    // in future release JitCompilerTask when compile finish
    JitPassManager *passManager = passManager_.release();
    delete passManager;
}

bool JitCompilerTask::Finalize(JitTask *jitTask)
{
    if (jitTask == nullptr) {
        return false;
    }
    if (compilerTier_ == CompilerTier::BASELINE) {
        baselineCompiler_->CollectMemoryCodeInfos(jitTask->GetMachineCodeDesc());
        return true;
    }
    jitCodeGenerator_->JitCreateLitecgModule();
    passManager_->RunCg();
    jitCodeGenerator_->GetMemoryCodeInfos(jitTask->GetMachineCodeDesc());
    ReleaseJitPassManager();
    return true;
}

void InitJitCompiler(JSRuntimeOptions options)
{
    JitCompiler *jitCompiler = JitCompiler::GetInstance(&options);
    jitCompiler->Init(options);
}

void *CreateJitCompilerTask(JitTask *jitTask)
{
    if (jitTask == nullptr) {
        return nullptr;
    }
    return JitCompilerTask::CreateJitCompilerTask(jitTask);
}

bool JitCompile(void *compilerTask, JitTask *jitTask)
{
    if (jitTask == nullptr || compilerTask == nullptr) {
        return false;
    }
    auto jitCompilerTask = reinterpret_cast<JitCompilerTask*>(compilerTask);
    return jitCompilerTask->Compile();
}

bool JitFinalize(void *compilerTask, JitTask *jitTask)
{
    if (jitTask == nullptr || compilerTask == nullptr) {
        return false;
    }
    auto jitCompilerTask = reinterpret_cast<JitCompilerTask*>(compilerTask);
    return jitCompilerTask->Finalize(jitTask);
}

void DeleteJitCompile(void *handle)
{
    if (handle == nullptr) {
        return;
    }
    delete reinterpret_cast<JitCompilerTask*>(handle);
}
}  // namespace panda::ecmascript::kungfu
