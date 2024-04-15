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

#include "ecmascript/jit/jit.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/dfx/vmstat/jit_preheat_profiler.h"

namespace panda::ecmascript {
void (*Jit::initJitCompiler_)(JSRuntimeOptions options) = nullptr;
bool(*Jit::jitCompile_)(void*, JitTask*) = nullptr;
bool(*Jit::jitFinalize_)(void*, JitTask*) = nullptr;
void*(*Jit::createJitCompilerTask_)(JitTask*) = nullptr;
void(*Jit::deleteJitCompile_)(void*) = nullptr;
void *Jit::libHandle_ = nullptr;

Jit *Jit::GetInstance()
{
    static Jit instance_;
    return &instance_;
}

void Jit::SetEnableOrDisable(const JSRuntimeOptions &options, bool isEnable)
{
    LockHolder holder(setEnableLock_);
    if (options.IsEnableAPPJIT()) {
        // temporary for app jit options test.
        LOG_JIT(DEBUG) << (isEnable ? "jit is enable" : "jit is disable");
        return;
    }
    if (!isEnable) {
        jitEnable_ = false;
        return;
    }
    if (!initialized_) {
        Initialize();
    }
    if (initialized_ && !jitEnable_) {
        jitEnable_ = true;
        initJitCompiler_(options);
        JitTaskpool::GetCurrentTaskpool()->Initialize();
    }
}

void Jit::Destroy()
{
    if (!initialized_) {
        return;
    }

    LockHolder holder(setEnableLock_);

    JitTaskpool::GetCurrentTaskpool()->Destroy();
    initialized_ = false;
    jitEnable_ = false;
    if (libHandle_ != nullptr) {
        CloseLib(libHandle_);
        libHandle_ = nullptr;
    }
}

bool Jit::IsEnable()
{
    return jitEnable_;
}

void Jit::Initialize()
{
    static const std::string CREATEJITCOMPILETASK = "CreateJitCompilerTask";
    static const std::string JITCOMPILEINIT = "InitJitCompiler";
    static const std::string JITCOMPILE = "JitCompile";
    static const std::string JITFINALIZE = "JitFinalize";
    static const std::string DELETEJITCOMPILE = "DeleteJitCompile";
    static const std::string LIBARK_JSOPTIMIZER = "libark_jsoptimizer.so";

    libHandle_ = LoadLib(LIBARK_JSOPTIMIZER);
    if (libHandle_ == nullptr) {
        LOG_JIT(ERROR) << "jit dlopen libark_jsoptimizer.so failed";
        return;
    }

    initJitCompiler_ = reinterpret_cast<void(*)(JSRuntimeOptions)>(FindSymbol(libHandle_, JITCOMPILEINIT.c_str()));
    if (initJitCompiler_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol initJitCompiler";
        return;
    }
    jitCompile_ = reinterpret_cast<bool(*)(void*, JitTask*)>(FindSymbol(libHandle_, JITCOMPILE.c_str()));
    if (jitCompile_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol jitCompile";
        return;
    }

    jitFinalize_ = reinterpret_cast<bool(*)(void*, JitTask*)>(FindSymbol(libHandle_, JITFINALIZE.c_str()));
    if (jitFinalize_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol jitFinalize";
        return;
    }

    createJitCompilerTask_ = reinterpret_cast<void*(*)(JitTask*)>(FindSymbol(libHandle_,
        CREATEJITCOMPILETASK.c_str()));
    if (createJitCompilerTask_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol createJitCompilertask";
        return;
    }

    deleteJitCompile_ = reinterpret_cast<void(*)(void*)>(FindSymbol(libHandle_, DELETEJITCOMPILE.c_str()));
    if (deleteJitCompile_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol deleteJitCompile";
        return;
    }
    initialized_= true;
    return;
}

Jit::~Jit()
{
}

bool Jit::SupportJIT(Method *method)
{
    FunctionKind kind = method->GetFunctionKind();
    switch (kind) {
        case FunctionKind::NORMAL_FUNCTION:
        case FunctionKind::BASE_CONSTRUCTOR:
        case FunctionKind::ARROW_FUNCTION:
        case FunctionKind::CLASS_CONSTRUCTOR:
            return true;
        default:
            return false;
    }
}

void Jit::DeleteJitCompile(void *compiler)
{
    deleteJitCompile_(compiler);
}

void Jit::CountInterpExecFuncs(JSHandle<JSFunction> &jsFunction)
{
    Method *method = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
    CString fileDesc = method->GetJSPandaFile()->GetJSPandaFileDesc();
    CString methodInfo = fileDesc + ":" + CString(method->GetMethodName());
    auto &profMap = JitPreheatProfiler::GetInstance()->profMap_;
    if (profMap.find(methodInfo) == profMap.end()) {
        profMap.insert({methodInfo, false});
    }
}

void Jit::Compile(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, int32_t offset, JitCompileMode mode)
{
    auto jit = Jit::GetInstance();
    if (!jit->IsEnable()) {
        return;
    }

    if (!vm->IsEnableOsr() && offset != MachineCode::INVALID_OSR_OFFSET) {
        return;
    }

    Method *method = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
    CString fileDesc = method->GetJSPandaFile()->GetJSPandaFileDesc();
#if ECMASCRIPT_ENABLE_JIT_PREHEAT_PROFILER
    CString methodInfo = fileDesc + ":" + CString(method->GetMethodName());
#else
    uint32_t codeSize = method->GetCodeSize();
    CString methodInfo = method->GetRecordNameStr() + "." + CString(method->GetMethodName()) + ", at:" + fileDesc +
        ", code size:" + ToCString(codeSize);
    constexpr uint32_t maxSize = 9000;
    if (codeSize > maxSize) {
        LOG_JIT(DEBUG) << "skip jit task, as too large:" << methodInfo;
        return;
    }
#endif
    if (vm->GetJSThread()->IsMachineCodeLowMemory()) {
        LOG_JIT(DEBUG) << "skip jit task, as low code memory:" << methodInfo;
        return;
    }
    bool isJSSharedFunction = jsFunction.GetTaggedValue().IsJSSharedFunction();
    if (!jit->SupportJIT(method) || isJSSharedFunction) {
        FunctionKind kind = method->GetFunctionKind();
#if ECMASCRIPT_ENABLE_JIT_PREHEAT_PROFILER
        LOG_JIT(ERROR) << "method does not support jit:" << methodInfo << ", kind:" << static_cast<int>(kind)
            <<", JSSharedFunction:" << isJSSharedFunction;
#else
        LOG_JIT(INFO) << "method does not support jit:" << methodInfo << ", kind:" << static_cast<int>(kind)
            <<", JSSharedFunction:" << isJSSharedFunction;
#endif
        return;
    }

    if (jsFunction->GetMachineCode() == JSTaggedValue::Hole()) {
        LOG_JIT(DEBUG) << "skip method, as it compiling:" << methodInfo;
#if ECMASCRIPT_ENABLE_JIT_PREHEAT_PROFILER
        auto &profMap = JitPreheatProfiler::GetInstance()->profMap_;
        if (profMap.find(methodInfo) != profMap.end()) {
            profMap.erase(methodInfo);
        }
#endif
        return;
    }

    if (jsFunction->GetMachineCode() != JSTaggedValue::Undefined()) {
        MachineCode *machineCode = MachineCode::Cast(jsFunction->GetMachineCode().GetTaggedObject());
        if (machineCode->GetOSROffset() == MachineCode::INVALID_OSR_OFFSET) {
            LOG_JIT(DEBUG) << "skip method, as it has been jit compiled:" << methodInfo;
            return;
        }
    }

    // using hole value to indecate compiling. todo: reset when failed
    jsFunction->SetMachineCode(vm->GetJSThread(), JSTaggedValue::Hole());
    {
        CString msg = "compile method:" + methodInfo + ", in work thread";
        TimeScope scope(msg);
        JitTaskpool::GetCurrentTaskpool()->WaitForJitTaskPoolReady();
        EcmaVM *compilerVm = JitTaskpool::GetCurrentTaskpool()->GetCompilerVm();
        std::shared_ptr<JitTask> jitTask = std::make_shared<JitTask>(vm->GetJSThread(), compilerVm->GetJSThread(),
            jit, jsFunction, methodInfo, offset, vm->GetJSThread()->GetThreadId(), mode);

        jitTask->PrepareCompile();
        JitTaskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<JitTask::AsyncTask>(jitTask, vm->GetJSThread()->GetThreadId()));
        if (mode == SYNC) {
            // sync mode, also compile in taskpool as litecg unsupport parallel compile,
            // wait task compile finish then install code
            jitTask->WaitFinish();
            jitTask->InstallCode();
        }
    }
}

void Jit::RequestInstallCode(std::shared_ptr<JitTask> jitTask)
{
    LockHolder holder(installJitTasksDequeMtx_);
    auto &taskQueue = installJitTasks_[jitTask->GetTaskThreadId()];
    taskQueue.push_back(jitTask);

    // set
    jitTask->GetHostThread()->SetInstallMachineCode(true);
    jitTask->GetHostThread()->SetCheckSafePointStatus();
}

void Jit::InstallTasks(uint32_t threadId)
{
    LockHolder holder(installJitTasksDequeMtx_);
    auto &taskQueue = installJitTasks_[threadId];
    for (auto it = taskQueue.begin(); it != taskQueue.end(); it++) {
        std::shared_ptr<JitTask> task = *it;
        // check task state
        task->InstallCode();
    }
    taskQueue.clear();
}

bool Jit::JitCompile(void *compiler, JitTask *jitTask)
{
    ASSERT(jitCompile_ != nullptr);
    return jitCompile_(compiler, jitTask);
}

bool Jit::JitFinalize(void *compiler, JitTask *jitTask)
{
    ASSERT(jitFinalize_ != nullptr);
    return jitFinalize_(compiler, jitTask);
}

void *Jit::CreateJitCompilerTask(JitTask *jitTask)
{
    ASSERT(createJitCompilerTask_ != nullptr);
    return createJitCompilerTask_(jitTask);
}

void Jit::ClearTask(const std::function<bool(Task *task)> &checkClear)
{
    JitTaskpool::GetCurrentTaskpool()->ForEachTask([&checkClear](Task *task) {
        JitTask::AsyncTask *asyncTask = static_cast<JitTask::AsyncTask*>(task);
        if (checkClear(asyncTask)) {
            asyncTask->Terminated();
        }

        if (asyncTask->IsRunning()) {
            asyncTask->WaitFinish();
        }
    });
}

void Jit::ClearTask(EcmaContext *ecmaContext)
{
    ClearTask([ecmaContext](Task *task) {
        JitTask::AsyncTask *asyncTask = static_cast<JitTask::AsyncTask*>(task);
        return ecmaContext == asyncTask->GetEcmaContext();
    });
}

void Jit::ClearTaskWithVm(EcmaVM *vm)
{
    ClearTask([vm](Task *task) {
        JitTask::AsyncTask *asyncTask = static_cast<JitTask::AsyncTask*>(task);
        return vm == asyncTask->GetHostVM();
    });

    LockHolder holder(installJitTasksDequeMtx_);
    auto &taskQueue = installJitTasks_[vm->GetJSThread()->GetThreadId()];
    taskQueue.clear();
}

void Jit::CheckMechineCodeSpaceMemory(JSThread *thread, int remainSize)
{
    if (!thread->IsMachineCodeLowMemory()) {
        return;
    }
    if (remainSize > MIN_CODE_SPACE_SIZE) {
        thread->SetMachineCodeLowMemory(false);
    }
}

Jit::TimeScope::~TimeScope()
{
    if (outPutLog_) {
        LOG_JIT(INFO) << message_ << ": " << TotalSpentTime() << "ms";
    }
}
}  // namespace panda::ecmascript
