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
#include "ecmascript/module/js_module_deregister.h"

#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/module_path_helper.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;

void ModuleDeregister::FreeModuleRecord([[maybe_unused]] void *env, void *pointer, void *hint)
{
    if (pointer == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Lacking deregister module's name.";
        return;
    }
    auto thread = reinterpret_cast<JSThread* >(hint);

    // pointer is module's name, which will be deregistered.
    JSTaggedValue moduleVal =
        thread->GetCurrentEcmaContext()->GetModuleManager()->HostGetImportedModule(pointer);
    if (moduleVal.IsUndefined()) {
        return;
    }
    JSHandle<SourceTextModule> module(thread, SourceTextModule::Cast(moduleVal.GetTaggedObject()));
    LoadingTypes type = module->GetLoadingTypes();
    CString recordNameStr = SourceTextModule::GetModuleName(module.GetTaggedValue());
    if (type != LoadingTypes::DYNAMITC_MODULE) {
        LOG_FULL(INFO) << "free stable module's ModuleNameSpace" << recordNameStr;
    }
    NativeAreaAllocator* allocator = thread->GetEcmaVM()->GetNativeAreaAllocator();
    allocator->FreeBuffer(pointer);
    if (type == LoadingTypes::DYNAMITC_MODULE) {
        std::set<CString> decreaseModule = {recordNameStr};
        DecreaseRegisterCounts(thread, module, decreaseModule);
        uint16_t counts = module->GetRegisterCounts();
        if (counts == 0) {
            thread->GetEcmaVM()->RemoveFromDeregisterModuleList(recordNameStr);
        }
        LOG_FULL(INFO) << "try to remove module " << recordNameStr << ", register counts is " << counts;
    }
}

void ModuleDeregister::ReviseLoadedModuleCount(JSThread *thread, const CString &moduleName)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    if (!moduleManager->IsLocalModuleLoaded(moduleName)) {
        return;
    }
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule(moduleName);

    LoadingTypes type = module->GetLoadingTypes();
    // do not change stable module's RegisterCounts.
    if (type == LoadingTypes::STABLE_MODULE) {
        return;
    }
    if (!vm->ContainInDeregisterModuleList(moduleName)) {
        std::set<CString> increaseModule = {moduleName};
        IncreaseRegisterCounts(thread, module, increaseModule);
    }
}

void ModuleDeregister::RemoveModule(JSThread *thread, JSHandle<SourceTextModule> module)
{
    CString recordName = SourceTextModule::GetModuleName(module.GetTaggedValue());
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    if (!thread->GetEcmaVM()->IsWorkerThread() &&
        (module->GetTypes() == ModuleTypes::APP_MODULE || module->GetTypes() == ModuleTypes::OHOS_MODULE)) {
        if (TryToRemoveSO(thread, module)) {
            LOG_FULL(INFO) << "Remove native module " << recordName << " successfully.";
        } else {
            LOG_FULL(INFO) << "Remove native module " << recordName << " failed.";
        }
    }
    moduleManager->RemoveModuleFromCache(recordName);
}

void ModuleDeregister::IncreaseRegisterCounts(JSThread *thread, JSHandle<SourceTextModule> module,
    std::set<CString> &increaseModule)
{
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            required.Update(requestedModules->Get(idx));
            JSMutableHandle<SourceTextModule> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
            const CString moduleRecordName = module->GetEcmaModuleRecordNameString();
            CString moduleName;
            if (moduleRecordName.empty()) {
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModule(thread, module, required);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
                moduleName = requiredModule->GetEcmaModuleFilenameString();
            } else {
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, required);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
                moduleName = requiredModule->GetEcmaModuleRecordNameString();
            }
            if (increaseModule.find(moduleName) != increaseModule.end()) {
                LOG_FULL(DEBUG) << "Find module cyclical loading, stop increasing.";
                requiredModule->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
                return;
            }
            increaseModule.emplace(moduleName);
            LoadingTypes type = requiredModule->GetLoadingTypes();
            if (type == LoadingTypes::DYNAMITC_MODULE) {
                IncreaseRegisterCounts(thread, requiredModule, increaseModule);
            }
        }
    }

    if (module->GetLoadingTypes() == LoadingTypes::STABLE_MODULE) {
        return;
    }
    uint16_t registerNum = module->GetRegisterCounts();
    if (registerNum == UINT16_MAX) {
        module->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
        return;
    }
    module->SetRegisterCounts(registerNum + 1);
}

void ModuleDeregister::DecreaseRegisterCounts(JSThread *thread, JSHandle<SourceTextModule> module,
    std::set<CString> &decreaseModule)
{
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            required.Update(requestedModules->Get(idx));
            JSMutableHandle<SourceTextModule> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
            const CString moduleRecordName = module->GetEcmaModuleRecordNameString();
            CString moduleName;
            if (moduleRecordName.empty()) {
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModule(thread, module, required);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
                moduleName = requiredModule->GetEcmaModuleFilenameString();
            } else {
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, required);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
                moduleName = requiredModule->GetEcmaModuleRecordNameString();
            }
            if (decreaseModule.find(moduleName) != decreaseModule.end()) {
                LOG_FULL(DEBUG) << "Find module cyclical loading, stop increasing.";
                requiredModule->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
                return;
            }
            decreaseModule.emplace(moduleName);
            LoadingTypes type = requiredModule->GetLoadingTypes();
            if (type == LoadingTypes::DYNAMITC_MODULE) {
                DecreaseRegisterCounts(thread, requiredModule, decreaseModule);
            }
        }
    }

    if (module->GetLoadingTypes() != LoadingTypes::DYNAMITC_MODULE) {
        return;
    }
    uint16_t num = module->GetRegisterCounts();
    if (num == 0) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "moduleNameSpace can not be uninstalled";
    }

    uint16_t registerNum = num - 1;
    if (registerNum == 0) {
        LOG_FULL(INFO) << "try to remove module " << SourceTextModule::GetModuleName(module.GetTaggedValue());
        RemoveModule(thread, module);
    }
    module->SetRegisterCounts(registerNum);
}
} // namespace panda::ecmascript
