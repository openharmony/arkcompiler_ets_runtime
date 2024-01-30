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
void Jit::Initialize()
{
    if (!vm_->IsEnableJit()) {
        return;
    }
    static const std::string CREATEJITCOMPILE = "CreateJitCompiler";
    static const std::string JITCOMPILE = "JitCompile";
    static const std::string JITFINALIZE = "JitFinalize";
    static const std::string DELETEJITCOMPILE = "DeleteJitCompile";
    static const std::string LIBARK_JSOPTIMIZER = "libark_jsoptimizer.so";

    libHandle_ = LoadLib(LIBARK_JSOPTIMIZER);
    if (libHandle_ == nullptr) {
        LOG_JIT(ERROR) << "jit dlopen libark_jsoptimizer.so failed";
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

    createJitCompiler_ = reinterpret_cast<void*(*)(EcmaVM*, JitTask*)>(FindSymbol(libHandle_,
        CREATEJITCOMPILE.c_str()));
    if (createJitCompiler_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol createJitCompiler";
        return;
    }

    deleteJitCompile_ = reinterpret_cast<void(*)(void*)>(FindSymbol(libHandle_, DELETEJITCOMPILE.c_str()));
    if (createJitCompiler_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol deleteJitCompile";
        return;
    }
    initialized_= true;
    vm_->GetJSThread()->SwitchJitProfileStubsIfNeeded();
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
    if (!vm->IsEnableJit()) {
        return;
    }
    auto jit = vm->GetJit();
    if (!jit->IsInitialized()) {
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
    if (jsFunction->GetMachineCode() != JSTaggedValue::Undefined()) {
        LOG_JIT(DEBUG) << "skip method, as it has been jit compiled:" << methodName;
        return;
    }

    if (jit->IsCompiling(jsFunction)) {
        LOG_JIT(DEBUG) << "skip method, as it compiling:" << methodName;
        return;
    }

    LOG_JIT(DEBUG) << "start compile:" << methodName << ", kind:" << static_cast<int>(kind) <<
        ", mode:" << ((mode == SYNC) ? "sync" : "async");

    {
        CString msg = "compile method:" + methodName + ", in work thread";
        Scope scope(msg);

        ASSERT(jit->createJitCompiler_ != nullptr);
        ASSERT(jit->deleteJitCompile_ != nullptr);

        JitTask *jitTask = new JitTask(vm, jit, jsFunction);
        jitTask->SetMethodInfo(methodName);
        jit->AddCompilingTask(jitTask);

        void *compiler = jit->createJitCompiler_(vm, jitTask);
        jitTask->SetCompiler(compiler);

        jitTask->Optimize();

        if (mode == SYNC) {
            // cg
            jitTask->Finalize();
            jitTask->InstallCode();
            jit->RemoveCompilingTask(jitTask);
            // free
            delete jitTask;
        } else {
            jit->AddAsyncCompileTask(jitTask);
        }
    }
}

void Jit::AddCompilingTask(JitTask *jitTask)
{
    compilingJitTasks_.push_back(jitTask);
}

void Jit::RemoveCompilingTask(JitTask *jitTask)
{
    auto findIt = std::find(compilingJitTasks_.begin(), compilingJitTasks_.end(), jitTask);
    ASSERT(findIt != compilingJitTasks_.end());
    compilingJitTasks_.erase(findIt);
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
            std::make_unique<JitTask::AsyncTask>(jitTask, vm_->GetJSThread()->GetThreadId()));
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
    installJitTasks_.push_back(jitTask);

    // set
    vm_->GetJSThread()->SetInstallMachineCode(true);
    vm_->GetJSThread()->SetCheckSafePointStatus();
}

bool Jit::IsCompiling(JSHandle<JSFunction> &jsFunction)
{
    Method *srcMethod = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
    for (auto it = compilingJitTasks_.begin(); it != compilingJitTasks_.end(); it++) {
        JitTask *task = *it;
        Method *compilingMethod = Method::Cast(task->GetJsFunction()->GetMethod().GetTaggedObject());
        if (srcMethod == compilingMethod) {
            return true;
        }
    }
    return false;
}

void Jit::InstallTasksWithoutClearFlag()
{
    LockHolder holder(installJitTasksDequeMtx_);
    for (auto it = installJitTasks_.begin(); it != installJitTasks_.end(); it++) {
        JitTask *task = *it;
        // check task state
        task->InstallCode();
        RemoveCompilingTask(task);
        delete task;
    }
    installJitTasks_.clear();
}

void Jit::InstallTasks()
{
    InstallTasksWithoutClearFlag();
    // clear flag
    vm_->GetJSThread()->SetInstallMachineCode(false);
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

Jit::Scope::~Scope()
{
    LOG_JIT(INFO) << message_ << ": " << TotalSpentTime() << "ms";
}
}  // namespace panda::ecmascript
