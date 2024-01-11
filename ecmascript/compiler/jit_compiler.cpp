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

#include "ecmascript/log.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript::kungfu {
JitCompilationOptions::JitCompilationOptions(JSRuntimeOptions &runtimeOptions, EcmaVM *vm)
{
    triple_ = runtimeOptions.GetTargetTriple();
    optLevel_ = runtimeOptions.GetOptLevel();
    relocMode_ = runtimeOptions.GetRelocMode();
    logOption_ = runtimeOptions.GetCompilerLogOption();
    logMethodsList_ = runtimeOptions.GetMethodsListForLog();
    compilerLogTime_ = runtimeOptions.IsEnableCompilerLogTime();
    hotnessThreshold_ = runtimeOptions.GetPGOHotnessThreshold();
    profilerIn_ = std::string(runtimeOptions.GetPGOProfilerPath());
    isEnableArrayBoundsCheckElimination_ = runtimeOptions.IsEnableArrayBoundsCheckElimination();
    isEnableTypeLowering_ = runtimeOptions.IsEnableTypeLowering();
    isEnableEarlyElimination_ = runtimeOptions.IsEnableEarlyElimination();
    isEnableLaterElimination_ = runtimeOptions.IsEnableLaterElimination();
    isEnableValueNumbering_ = runtimeOptions.IsEnableValueNumbering();
    isEnableOptInlining_ = runtimeOptions.IsEnableOptInlining();
    isEnableOptString_ = runtimeOptions.IsEnableOptString();
    isEnableTypeInfer_ =
        isEnableTypeLowering_ || vm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->AssertTypes();
    isEnableOptPGOType_ = runtimeOptions.IsEnableOptPGOType();
    isEnableOptTrackField_ = runtimeOptions.IsEnableOptTrackField();
    isEnableOptLoopPeeling_ = runtimeOptions.IsEnableOptLoopPeeling();
    isEnableOptOnHeapCheck_ = runtimeOptions.IsEnableOptOnHeapCheck();
    isEnableOptLoopInvariantCodeMotion_ = runtimeOptions.IsEnableOptLoopInvariantCodeMotion();
    isEnableCollectLiteralInfo_ = false;
    isEnableOptConstantFolding_ = runtimeOptions.IsEnableOptConstantFolding();
    isEnableLexenvSpecialization_ = runtimeOptions.IsEnableLexenvSpecialization();
    isEnableNativeInline_ = runtimeOptions.IsEnableNativeInline();
    isEnableLoweringBuiltin_ = runtimeOptions.IsEnableLoweringBuiltin();
}

void JitCompiler::Init()
{
    log_.SetEnableCompilerLogTime(jitOptions_.compilerLogTime_);
    PassOptions::Builder optionsBuilder;
    passOptions_ =
        optionsBuilder.EnableArrayBoundsCheckElimination(jitOptions_.isEnableArrayBoundsCheckElimination_)
            .EnableTypeLowering(jitOptions_.isEnableTypeLowering_)
            .EnableEarlyElimination(jitOptions_.isEnableEarlyElimination_)
            .EnableLaterElimination(jitOptions_.isEnableLaterElimination_)
            .EnableValueNumbering(jitOptions_.isEnableValueNumbering_)
            .EnableTypeInfer(jitOptions_.isEnableTypeInfer_)
            .EnableOptInlining(jitOptions_.isEnableOptInlining_)
            .EnableOptString(jitOptions_.isEnableOptString_)
            .EnableOptPGOType(jitOptions_.isEnableOptPGOType_)
            .EnableOptTrackField(jitOptions_.isEnableOptTrackField_)
            .EnableOptLoopPeeling(jitOptions_.isEnableOptLoopPeeling_)
            .EnableOptLoopInvariantCodeMotion(jitOptions_.isEnableOptLoopInvariantCodeMotion_)
            .EnableCollectLiteralInfo(jitOptions_.isEnableCollectLiteralInfo_)
            .EnableOptConstantFolding(jitOptions_.isEnableOptConstantFolding_)
            .EnableLexenvSpecialization(jitOptions_.isEnableLexenvSpecialization_)
            .EnableInlineNative(jitOptions_.isEnableNativeInline_)
            .EnableLoweringBuiltin(jitOptions_.isEnableLoweringBuiltin_)
            .Build();
    passManager_ = new JitPassManager(vm_,
                                      jitOptions_.triple_,
                                      jitOptions_.optLevel_,
                                      jitOptions_.relocMode_,
                                      &log_,
                                      &logList_,
                                      profilerDecoder_,
                                      &passOptions_);
    aotFileGenerator_ = new AOTFileGenerator(&log_, &logList_, vm_, jitOptions_.triple_,
        vm_->GetJSOptions().IsCompilerEnableLiteCG());
}

JitCompiler *JitCompiler::Create(EcmaVM *vm, JitTask *jitTask)
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    TSManager *tsm = new TSManager(vm);
    vm->GetJSThread()->GetCurrentEcmaContext()->SetTSManager(tsm);
    vm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->Initialize();

    auto jitCompiler = new JitCompiler(vm, jitTask->GetJsFunction());
    return jitCompiler;
}

JitCompiler::~JitCompiler()
{
    if (passManager_ != nullptr) {
        delete passManager_;
        passManager_ = nullptr;
    }
    if (aotFileGenerator_ != nullptr) {
        delete aotFileGenerator_;
        aotFileGenerator_ = nullptr;
    }
}

bool JitCompiler::Compile()
{
    return passManager_->Compile(jsFunction_, *aotFileGenerator_);
}

bool JitCompiler::Finalize(JitTask *jitTask)
{
    aotFileGenerator_->JitCreateLitecgModule();
    passManager_->RunCg();
    aotFileGenerator_->GetMemoryCodeInfos(jitTask->GetMachineCodeDesc());
    return true;
}

void *CreateJitCompiler(EcmaVM *vm, JitTask *jitTask)
{
    auto jitCompiler = JitCompiler::Create(vm, jitTask);
    return jitCompiler;
}

bool JitCompile(void *compiler, JitTask *jitTask [[maybe_unused]])
{
    ASSERT(jitTask != nullptr);
    ASSERT(compiler != nullptr);
    auto jitCompiler = reinterpret_cast<JitCompiler *>(compiler);
    bool ret = jitCompiler->Compile();

    return ret;
}

bool JitFinalize(void *compiler, JitTask *jitTask)
{
    ASSERT(jitTask != nullptr);
    ASSERT(compiler != nullptr);
    auto jitCompiler = reinterpret_cast<JitCompiler *>(compiler);
    bool ret = jitCompiler->Finalize(jitTask);
    return ret;
}

void DeleteJitCompile(void *handle)
{
    ASSERT(handle != nullptr);
    delete reinterpret_cast<JitCompiler *>(handle);
}
}  // namespace panda::ecmascript::kungfu
