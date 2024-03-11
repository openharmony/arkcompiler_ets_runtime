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

#include "ecmascript/jit/jit_task.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/global_env.h"
#include "ecmascript/compiler/aot_file/func_entry_des.h"

namespace panda::ecmascript {
void JitTask::PrepareCompile()
{
    PersistentHandle();
    compilerTask_ = jit_->CreateJitCompilerTask(this);
}

void JitTask::Optimize()
{
    bool res = jit_->JitCompile(compilerTask_, this);
    if (!res) {
        SetCompileFailed();
    }
}

void JitTask::Finalize()
{
    if (!IsCompileSuccess()) {
        return;
    }

    bool res = jit_->JitFinalize(compilerTask_, this);
    if (!res) {
        SetCompileFailed();
    }
}

void JitTask::InstallCode()
{
    if (!IsCompileSuccess()) {
        return;
    }
    JSHandle<Method> methodHandle(vm_->GetJSThread(), Method::Cast(jsFunction_->GetMethod().GetTaggedObject()));

    size_t funcEntryDesSizeAlign = AlignUp(codeDesc_.funcEntryDesSize, MachineCode::TEXT_ALIGN);

    size_t rodataSizeBeforeTextAlign = AlignUp(codeDesc_.rodataSizeBeforeText, MachineCode::TEXT_ALIGN);
    size_t codeSizeAlign = AlignUp(codeDesc_.codeSize, MachineCode::DATA_ALIGN);
    size_t rodataSizeAfterTextAlign = AlignUp(codeDesc_.rodataSizeAfterText, MachineCode::DATA_ALIGN);

    size_t stackMapSizeAlign = AlignUp(codeDesc_.stackMapSize, MachineCode::DATA_ALIGN);

    size_t size = funcEntryDesSizeAlign + rodataSizeBeforeTextAlign + codeSizeAlign + rodataSizeAfterTextAlign +
        stackMapSizeAlign;

    JSHandle<MachineCode> machineCodeObj = vm_->GetFactory()->NewMachineCodeObject(size, &codeDesc_, methodHandle);
    // oom?
    uintptr_t codeAddr = machineCodeObj->GetFuncAddr();
    FuncEntryDes *funcEntryDes = reinterpret_cast<FuncEntryDes*>(machineCodeObj->GetFuncEntryDes());
    jsFunction_->SetCompiledFuncEntry(codeAddr, funcEntryDes->isFastCall_);
    methodHandle->SetDeoptThreshold(vm_->GetJSOptions().GetDeoptThreshold());
    jsFunction_->SetMachineCode(vm_->GetJSThread(), machineCodeObj);

    LOG_JIT(DEBUG) << "Install machine code:" << GetMethodInfo();
}

void JitTask::PersistentHandle()
{
    // transfer to global ref
    GlobalHandleCollection globalHandleCollection(vm_->GetJSThread());
    JSHandle<JSFunction> persistentHandle =
        globalHandleCollection.NewHandle<JSFunction>(jsFunction_.GetTaggedType());
    SetJsFunction(persistentHandle);
}

void JitTask::ReleasePersistentHandle()
{
    GlobalHandleCollection globalHandleCollection(vm_->GetJSThread());
    globalHandleCollection.Dispose(jsFunction_);
}

JitTask::~JitTask()
{
    ReleasePersistentHandle();
    jit_->DeleteJitCompile(compilerTask_);
}

bool JitTask::AsyncTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    CString info = jitTask_->GetMethodInfo() + ", in task pool, id:" + ToCString(threadIndex) + ", time:";
    Jit::Scope scope(info);

    jitTask_->Finalize();

    // info main thread compile complete
    jitTask_->RequestInstallCode();

    // as now litecg has global value, so only compile one task at a time
    jitTask_->GetJit()->RemoveAsyncCompileTask(jitTask_);
    JitTask *jitTask = jitTask_->GetJit()->GetAsyncCompileTask();
    if (jitTask != nullptr) {
        Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<JitTask::AsyncTask>(jitTask, jitTask->GetVM()->GetJSThread()->GetThreadId()));
    }
    return true;
}
}  // namespace panda::ecmascript
