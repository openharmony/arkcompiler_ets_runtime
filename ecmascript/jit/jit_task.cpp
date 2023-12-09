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
void JitTask::Optimize()
{
    bool res = jit_->JitCompile(compiler_, this);
    if (!res) {
        SetCompileFailed();
    }
}

void JitTask::Finalize()
{
    if (!IsCompileSuccess()) {
        return;
    }

    bool res = jit_->JitFinalize(compiler_, this);
    if (!res) {
        SetCompileFailed();
    }
}

void JitTask::InstallCode()
{
    if (!IsCompileSuccess()) {
        return;
    }
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    JSHandle<Method> methodHandle(vm_->GetJSThread(), method);

    size_t funcEntryDesSizeAlign = AlignUp(codeDesc_.funcEntryDesSize, MachineCode::DATA_ALIGN);

    size_t rodataSizeBeforeTextAlign = AlignUp(codeDesc_.rodataSizeBeforeText, MachineCode::TEXT_ALIGN);
    size_t codeSizeAlign = AlignUp(codeDesc_.codeSize, MachineCode::DATA_ALIGN);
    size_t rodataSizeAfterTextAlign = AlignUp(codeDesc_.rodataSizeAfterText, MachineCode::DATA_ALIGN);

    size_t stackMapSizeAlign = AlignUp(codeDesc_.stackMapSize, MachineCode::DATA_ALIGN);

    size_t size = funcEntryDesSizeAlign + rodataSizeBeforeTextAlign + codeSizeAlign + rodataSizeAfterTextAlign +
        stackMapSizeAlign;

    JSHandle<MachineCode> machineCodeObj = vm_->GetFactory()->NewMachineCodeObject(size, &codeDesc_, methodHandle);
    // oom?
    method = methodHandle.GetObject<Method>();
    uintptr_t codeAddr = machineCodeObj->GetFuncAddr();
    FuncEntryDes *funcEntryDes = reinterpret_cast<FuncEntryDes*>(machineCodeObj->GetFuncEntryDes());
    method->SetCompiledFuncEntry(codeAddr, funcEntryDes->isFastCall_);
    method->SetDeoptThreshold(vm_->GetJSOptions().GetDeoptThreshold());

    JSHandle<JSHClass> oldHClass(vm_->GetJSThread(), jsFunction_->GetClass());
    methodHandle->SetMachineCode(vm_->GetJSThread(), machineCodeObj);

    CString fileDesc = method->GetJSPandaFile()->GetJSPandaFileDesc();
    CString methodName = method->GetRecordNameStr() + "." + CString(method->GetMethodName()) + ", at:" + fileDesc;
    LOG_JIT(DEBUG) <<"install machine code:" << methodName;
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
    jit_->DeleteJitCompile(compiler_);
}

bool JitTask::AsyncTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    jitTask_->Finalize();

    // info main thread compile complete
    jitTask_->RequestInstallCode();
    return true;
}
}  // namespace panda::ecmascript
