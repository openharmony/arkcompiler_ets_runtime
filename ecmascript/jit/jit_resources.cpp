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

#include "ecmascript/jit/jit_resources.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/ohos/jit_tools.h"

namespace panda::ecmascript {

void JitResources::Destroy()
{
    JitTaskpool::GetCurrentTaskpool()->Destroy();
    if (libHandle_ != nullptr) {
        CloseLib(libHandle_);
        libHandle_ = nullptr;
        initJitCompiler_ = nullptr;
        jitCompile_ = nullptr;
        jitFinalize_ = nullptr;
        createJitCompilerTask_ = nullptr;
        deleteJitCompilerTask_ = nullptr;
    }
}

bool JitResources::InitJitEnv(const JSRuntimeOptions &options)
{
    jsRuntimeOptions_ = options;
    InitCompiler();
    InitJitTaskpool();
    return true;
}

bool JitResources::ResolveLib()
{
#if defined(OHOS_UNIT_TEST)
#else
    static const std::string CREATEJITCOMPILETASK = "CreateJitCompilerTask";
    static const std::string JITCOMPILEINIT = "InitJitCompiler";
    static const std::string JITCOMPILE = "JitCompile";
    static const std::string JITFINALIZE = "JitFinalize";
    static const std::string DELETEJITCOMPILERTASK = "DeleteJitCompilerTask";
    static const std::string LIBARK_JSOPTIMIZER = "libark_jsoptimizer.so";

    libHandle_ = LoadLib(LIBARK_JSOPTIMIZER);
    if (libHandle_ == nullptr) {
        char *error = LoadLibError();
        LOG_JIT(ERROR) << "jit dlopen libark_jsoptimizer.so failed, as:" <<
            ((error == nullptr) ? "unknown error" : error);
        return false;
    }

    initJitCompiler_ = reinterpret_cast<InitJitCompilerFuncType>(FindSymbol(libHandle_, JITCOMPILEINIT.c_str()));
    if (initJitCompiler_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol initJitCompiler";
        return false;
    }
    jitCompile_ = reinterpret_cast<JitCompileFuncType>(FindSymbol(libHandle_, JITCOMPILE.c_str()));
    if (jitCompile_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol jitCompile";
        return false;
    }

    jitFinalize_ = reinterpret_cast<JitFinalizeFuncType>(FindSymbol(libHandle_, JITFINALIZE.c_str()));
    if (jitFinalize_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol jitFinalize";
        return false;
    }

    createJitCompilerTask_ = reinterpret_cast<CreateJitCompilerTaskFuncType>(FindSymbol(libHandle_,
        CREATEJITCOMPILETASK.c_str()));
    if (createJitCompilerTask_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol createJitCompilertask";
        return false;
    }

    deleteJitCompilerTask_ = reinterpret_cast<DeleteJitCompilerTaskFuncType>(FindSymbol(libHandle_,
        DELETEJITCOMPILERTASK.c_str()));
    if (deleteJitCompilerTask_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol deleteJitCompile";
        return false;
    }
#endif
    libResolved_ = true;
    return true;
}

void JitResources::InitCompiler()
{
    if (initJitCompiler_ == nullptr) {
        return;
    }
    initJitCompiler_(jsRuntimeOptions_);
}

void JitResources::InitJitTaskpool()
{
    bool enableCodeSign = !ohos::JitTools::GetCodeSignDisable(jsRuntimeOptions_.GetDisableCodeSign());
    JitTaskpool::GetCurrentTaskpool()->Initialize(enableCodeSign);
}
}  // namespace panda::ecmascript
