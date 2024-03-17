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

namespace panda::ecmascript {
std::deque<JitTask*> Jit::asyncCompileJitTasks_;
Mutex Jit::asyncCompileJitTasksMtx_;
void (*Jit::initJitCompiler_)(EcmaVM *vm) = nullptr;
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

void Jit::SetEnable(const EcmaVM *vm)
{
    if (!initialized_) {
        Initialize();
    }
    if (initialized_ && !jitEnable_) {
        jitEnable_ = true;
        initJitCompiler_(const_cast<EcmaVM*>(vm));
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

    initJitCompiler_ = reinterpret_cast<void(*)(EcmaVM*)>(FindSymbol(libHandle_, JITCOMPILEINIT.c_str()));
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
    if (libHandle_ != nullptr) {
        CloseLib(libHandle_);
        libHandle_ = nullptr;
    }
}

bool Jit::SupportJIT(Method *method)
{
    FunctionKind kind = method->GetFunctionKind();
    switch (kind) {
        case FunctionKind::NORMAL_FUNCTION:
        case FunctionKind::BASE_CONSTRUCTOR:
        case FunctionKind::ARROW_FUNCTION:
            return true;
        default:
            return false;
    }
}

void Jit::DeleteJitCompile(void *compiler)
{
    deleteJitCompile_(compiler);
}

void Jit::Compile(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, JitCompileMode mode)
{
    auto jit = Jit::GetInstance();
    if (!jit->IsEnable()) {
        return;
    }

    Method *method = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
    CString fileDesc = method->GetJSPandaFile()->GetJSPandaFileDesc();
    FunctionKind kind = method->GetFunctionKind();
    CString methodName = method->GetRecordNameStr() + "." + CString(method->GetMethodName()) + ", at:" + fileDesc;
    if (!jit->SupportJIT(method)) {
        LOG_JIT(INFO) << "method does not support jit:" << methodName << ", kind:" << static_cast<int>(kind);
        return;
    }

    if (jsFunction->GetMachineCode() == JSTaggedValue::Hole()) {
        LOG_JIT(DEBUG) << "skip method, as it compiling:" << methodName;
        return;
    }
    if (jsFunction->GetMachineCode() != JSTaggedValue::Undefined()) {
        LOG_JIT(DEBUG) << "skip method, as it has been jit compiled:" << methodName;
        return;
    }
    // using hole value to indecate compiling. todo: reset when failed
    jsFunction->SetMachineCode(vm->GetJSThread(), JSTaggedValue::Hole());

    {
        CString msg = "compile method:" + methodName + ", in work thread";
        Scope scope(msg);

        JitTask *jitTask = new JitTask(vm, jit, jsFunction, methodName, vm->GetJSThread()->GetThreadId());

        jitTask->PrepareCompile();

        jitTask->Optimize();

        if (mode == SYNC) {
            // cg
            jitTask->Finalize();
            jitTask->InstallCode();
            // free
            delete jitTask;
        } else {
            jit->AddAsyncCompileTask(jitTask);
        }
    }
}

JitTask *Jit::GetAsyncCompileTask()
{
    LockHolder holder(asyncCompileJitTasksMtx_);
    if (asyncCompileJitTasks_.empty()) {
        return nullptr;
    } else {
        auto jitTask = asyncCompileJitTasks_.front();
        return jitTask;
    }
}

void Jit::AddAsyncCompileTask(JitTask *jitTask)
{
    LockHolder holder(asyncCompileJitTasksMtx_);
    if (asyncCompileJitTasks_.empty()) {
        Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<JitTask::AsyncTask>(jitTask, jitTask->GetVM()->GetJSThread()->GetThreadId()));
    }
    asyncCompileJitTasks_.push_back(jitTask);
}

void Jit::RemoveAsyncCompileTask([[maybe_unused]] JitTask *jitTask)
{
    LockHolder holder(asyncCompileJitTasksMtx_);
    ASSERT(!asyncCompileJitTasks_.empty());
    ASSERT(asyncCompileJitTasks_.front() == jitTask);
    asyncCompileJitTasks_.pop_front();
}

void Jit::RequestInstallCode(JitTask *jitTask)
{
    LockHolder holder(installJitTasksDequeMtx_);
    auto &taskQueue = installJitTasks_[jitTask->GetTaskThreadId()];
    taskQueue.push_back(jitTask);

    // set
    jitTask->GetVM()->GetJSThread()->SetInstallMachineCode(true);
    jitTask->GetVM()->GetJSThread()->SetCheckSafePointStatus();
}

void Jit::InstallTasks(uint32_t threadId)
{
    LockHolder holder(installJitTasksDequeMtx_);
    auto &taskQueue = installJitTasks_[threadId];
    for (auto it = taskQueue.begin(); it != taskQueue.end(); it++) {
        JitTask *task = *it;
        // check task state
        task->InstallCode();
        delete task;
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

Jit::Scope::~Scope()
{
    LOG_JIT(INFO) << message_ << ": " << TotalSpentTime() << "ms";
}
}  // namespace panda::ecmascript
