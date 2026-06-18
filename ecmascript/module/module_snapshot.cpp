/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/module/module_snapshot.h"

#include "ecmascript/base/config.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/signal_manager.h"
#include "ecmascript/serializer/file_deserializer.h"
#include "ecmascript/serializer/file_serializer.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#include "securec.h"
#include "zlib.h"

namespace panda::ecmascript {
void ModuleSnapshot::SerializeDataAndPostSavingJob(const EcmaVM *vm, const CString &path, const CString &version)
{
    LOG_ECMA(DEBUG) << "ModuleSnapshot::SerializeDataAndPostSavingJob " << path;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::SerializeDataAndPostSavingJob", "");
    CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
    if (FileExist(filePath.c_str())) {
        LOG_ECMA(INFO) << "Module serialize file already exist";
        return;
    }
    ModulesSnapshotHelper::MarkModuleSnapshotLoaded();
    JSThread *thread = vm->GetJSThread();
    std::unique_ptr<SerializeData> fileData = GetSerializeData(thread);
    if (fileData == nullptr) {
        return;
    }
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(vm->GetApplicationVersionCode(), version, "");
    common::Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SnapshotSaveTask>(
        thread->GetThreadId(), std::move(fileData), filePath, std::move(header), "ModuleSnapshot"));
}

bool ModuleSnapshot::DeserializeData(const EcmaVM *vm, const CString &path, const CString &version)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::DeserializeData", "");
    LOG_ECMA(DEBUG) << "ModuleSnapshot::DeserializeData";
    CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
    if (!FileExist(filePath.c_str())) {
        LOG_ECMA(INFO) << "ModuleSnapshot::DeserializeData Module serialize file doesn't exist: " << path;
        return false;
    }
    ModulesSnapshotHelper::MarkModuleSnapshotLoaded();
    JSThread *thread = vm->GetJSThread();
    std::unique_ptr<SerializeData> fileData = std::make_unique<SerializeData>(thread);
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(vm->GetApplicationVersionCode(), version, "");
    if (!ModulesSnapshotHelper::ReadDataFromFile(fileData, filePath, header, "ModuleSnapshot")) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::DeserializeData failed: " << filePath;
        ModulesSnapshotHelper::RemoveFile(filePath);
        return false;
    }
    FileDeserializer deserializer(thread, fileData.release());
    JSHandle<TaggedArray> deserializedModules = JSHandle<TaggedArray>::Cast(deserializer.ReadValue());
    uint32_t length = deserializedModules->GetLength();
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue module = deserializedModules->Get(thread, i);
        JSHandle<SourceTextModule> moduleHdl(thread, SourceTextModule::Cast(module.GetTaggedObject()));
        CString moduleName = SourceTextModule::GetModuleName(module);
        if (SourceTextModule::IsSharedModule(moduleHdl)) {
            SharedModuleManager::GetInstance()->AddToResolvedModulesAndCreateSharedModuleMutex(
                thread, moduleName, module);
            continue;
        }
        thread->GetModuleManager()->AddResolveImportedModule(moduleName, module);
    }
    LOG_ECMA(INFO) << "ModuleSnapshot::DeserializeData success";
    return true;
}

JSHandle<TaggedArray> ModuleSnapshot::GetModuleSerializeArray(JSThread *thread)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    uint32_t normalModuleSize = moduleManager->GetResolvedModulesSize();
    uint32_t sharedModuleSize = SharedModuleManager::GetInstance()->GetResolvedSharedModulesSize(thread);
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<TaggedArray> serializerArray = factory->NewTaggedArray(normalModuleSize + sharedModuleSize);
    moduleManager->AddNormalSerializeModule(thread, serializerArray, 0); // 0: start index
    SharedModuleManager::GetInstance()->AddSharedSerializeModule(thread, serializerArray, normalModuleSize);
    return serializerArray;
}

void ModuleSnapshot::RestoreUpdatedBinding(JSThread* thread, JSHandle<TaggedArray> serializeArray)
{
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<SourceTextModule> module(thread, globalConstants->GetUndefined());
    JSMutableHandle<ResolvedIndexBinding> indexBinding(thread, globalConstants->GetUndefined());
    JSMutableHandle<ResolvedRecordIndexBinding> recordIndexBinding(thread, globalConstants->GetUndefined());
    JSMutableHandle<TaggedArray> environment(thread, globalConstants->GetUndefined());
    for (uint32_t moduleIdx = 0; moduleIdx < serializeArray->GetLength(); ++moduleIdx) {
        module.Update(serializeArray->Get(thread, moduleIdx));
        JSTaggedValue moduleEnvironment = module->GetEnvironment(thread);
        if (moduleEnvironment.IsUndefined()) {
            continue;
        }
        environment.Update(moduleEnvironment);
        bool isShared = SourceTextModule::IsSharedModule(module);
        // check every binding and transfer from ResolvedIndexBinding to ResolvedBinding if binding updated.
        for (uint32_t bindingIdx = 0; bindingIdx < environment->GetLength(); bindingIdx++) {
            JSTaggedValue binding = environment->Get(thread, bindingIdx);
            if (binding.IsResolvedIndexBinding() &&
                ResolvedIndexBinding::Cast(binding)->GetIsUpdatedFromResolvedBinding()) {
                indexBinding.Update(binding);
                JSHandle<JSTaggedValue> nameBinding =
                    SourceTextModule::CreateBindingByIndexBinding(thread, indexBinding, isShared);
                environment->Set(thread, bindingIdx, nameBinding);
            } else if (binding.IsResolvedRecordIndexBinding() &&
                ResolvedRecordIndexBinding::Cast(binding)->GetIsUpdatedFromResolvedRecordBinding()) {
                recordIndexBinding.Update(binding);
                JSHandle<JSTaggedValue> nameBinding =
                    SourceTextModule::CreateBindingByRecordIndexBinding(thread, recordIndexBinding);
                environment->Set(thread, bindingIdx, nameBinding);
            }
        }
    }
}

std::unique_ptr<SerializeData> ModuleSnapshot::GetSerializeData(JSThread *thread)
{
    FileSerializer serializer(thread, FileSerializer::SourceTextModuleFilter);
    JSHandle<TaggedArray> serializeArray = GetModuleSerializeArray(thread);
    RestoreUpdatedBinding(thread, serializeArray);
    const GlobalEnvConstants *globalConstants = thread->GlobalConstants();
    if (!serializer.WriteValue(thread, JSHandle<JSTaggedValue>(serializeArray),
                               globalConstants->GetHandledUndefined(),
                               globalConstants->GetHandledUndefined())) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::GetSerializeData serialize failed";
        return nullptr;
    }
    std::unique_ptr<SerializeData> fileData = serializer.Release();
    return fileData;
}
} // namespace panda::ecmascript
