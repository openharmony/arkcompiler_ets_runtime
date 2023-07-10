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

#include "ecmascript/base/path_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;

void ModuleDeregister::FreeModuleRecord(void *pointer, void *hint)
{
    if (pointer == nullptr) {
        LOG_FULL(FATAL) << "Lacking deregister module's name.";
        return;
    }
    auto thread = reinterpret_cast<JSThread* >(hint);

    // pointer is module's name, which will be deregistered.
    JSHandle<SourceTextModule> module =
        thread->GetCurrentEcmaContext()->GetModuleManager()->HostGetImportedModule(pointer);

    LoadingTypes type = module->GetLoadingTypes();
    if (type != LoadingTypes::DYNAMITC_MODULE) {
        LOG_FULL(DEBUG) << "free stable module's ModuleNameSpace" <<
            ConvertToString(module->GetEcmaModuleRecordName()).c_str();
    }
    NativeAreaAllocator* allocator = thread->GetEcmaVM()->GetNativeAreaAllocator();
    allocator->FreeBuffer(pointer);
    if (type == LoadingTypes::DYNAMITC_MODULE) {
        DecreaseRegisterCounts(thread, module);
        if (module->GetRegisterCounts() == 0) {
            JSTaggedValue moduleRecordName = SourceTextModule::GetModuleName(module.GetTaggedValue());
            thread->GetEcmaVM()->RemoveFromDeregisterModuleList(ConvertToString(moduleRecordName));
        }
    }
}

void ModuleDeregister::ReviseLoadedModuleCount(JSThread *thread, JSTaggedValue moduleName)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule(moduleName);

    LoadingTypes type = module->GetLoadingTypes();
    // do not change stable module's RegisterCounts.
    if (type == LoadingTypes::STABLE_MODULE) {
        return;
    }
    if (!vm->ContainInDeregisterModuleList(ConvertToString(moduleName))) {
        IncreaseRegisterCounts(thread, module);
    }
}

void ModuleDeregister::RemoveModule(JSThread *thread, JSHandle<SourceTextModule> module)
{
    JSTaggedValue moduleRecordName = SourceTextModule::GetModuleName(module.GetTaggedValue());
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    if (module->GetTypes() == ModuleTypes::APP_MODULE) {
        if (TryToRemoveSO(thread, module)) {
            LOG_FULL(INFO) << "Remove native module " << ConvertToString(moduleRecordName).c_str() << " successfully.";
        }
        LOG_FULL(INFO) << "Remove native module " << ConvertToString(moduleRecordName).c_str() << " failed.";
    }
    JSHandle<NameDictionary> dict(thread, moduleManager->resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(moduleRecordName);
    LOG_ECMA_IF(entry == -1, FATAL) << "Can not get module: " << ConvertToString(moduleRecordName) <<
         ", when try to remove the module";

    moduleManager->resolvedModules_  = NameDictionary::Remove(thread, dict, entry).GetTaggedValue();
}

void ModuleDeregister::IncreaseRegisterCounts(JSThread *thread, JSHandle<SourceTextModule> module)
{
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            required.Update(requestedModules->Get(idx));
            JSMutableHandle<SourceTextModule> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
            JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
            if (moduleRecordName.IsUndefined()) {
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModule(thread, module, required);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
            } else {
                ASSERT(moduleRecordName.IsString());
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, required);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
            }

            LoadingTypes type = requiredModule->GetLoadingTypes();
            if (type == LoadingTypes::DYNAMITC_MODULE) {
                IncreaseRegisterCounts(thread, requiredModule);
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

void ModuleDeregister::DecreaseRegisterCounts(JSThread *thread, JSHandle<SourceTextModule> module)
{
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            required.Update(requestedModules->Get(idx));
            JSMutableHandle<SourceTextModule> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
            JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
            if (moduleRecordName.IsUndefined()) {
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModule(thread, module, required);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
            } else {
                ASSERT(moduleRecordName.IsString());
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, required);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
            }

            LoadingTypes type = requiredModule->GetLoadingTypes();
            if (type == LoadingTypes::DYNAMITC_MODULE) {
                DecreaseRegisterCounts(thread, requiredModule);
            }
        }
    }

    if (module->GetLoadingTypes() != LoadingTypes::DYNAMITC_MODULE) {
        return;
    }
    uint16_t num = module->GetRegisterCounts();
    if (num == 0) {
        LOG_FULL(FATAL) << "moduleNameSpace can not be uninstalled";
    }

    uint16_t registerNum = num - 1;
    if (registerNum == 0) {
        LOG_FULL(DEBUG) << "try to remove module " << ConvertToString(module->GetEcmaModuleRecordName()).c_str();
        RemoveModule(thread, module);
    }
    module->SetRegisterCounts(registerNum);
}
} // namespace panda::ecmascript
