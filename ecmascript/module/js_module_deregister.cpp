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
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/module_resolver.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;

namespace {
void ResetRegisterCountsForDeregisterInner(JSThread *thread, JSHandle<SourceTextModule> module,
                                           CUnorderedSet<CString, CStringHash> &visitedModules)
{
    CString moduleRecordName = SourceTextModule::GetModuleName(module.GetTaggedValue());
    if (!visitedModules.emplace(moduleRecordName).second) {
        return;
    }
    if (module->GetLoadingTypes() != LoadingTypes::DYNAMITC_MODULE) {
        return;
    }
    if (module->GetRequestedModules(thread).IsUndefined()) {
        return;
    }
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules(thread));
    for (size_t idx = 0; idx < requestedModules->GetLength(); idx++) {
        JSHandle<SourceTextModule> requiredModule =
            SourceTextModule::GetModuleFromCacheOrResolveNewOne(thread, module, requestedModules, idx);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        CString requiredModuleName = SourceTextModule::GetModuleName(requiredModule.GetTaggedValue());
        if (visitedModules.find(requiredModuleName) != visitedModules.end()) {
            continue;
        }
        JSHandle<JSTaggedValue> restoredModule =
            thread->GetModuleManager()->TryGetPendingRemovalModule(requiredModuleName);
        if (restoredModule->IsUndefined()) {
            ModuleDeregister::DisableMultiEntryDeregister(thread, requiredModule, ExecuteTypes::DYNAMIC);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            continue;
        }
        ModuleDeregister::InitForDeregisterModule(restoredModule, ExecuteTypes::DYNAMIC);
        ResetRegisterCountsForDeregisterInner(
            thread, JSHandle<SourceTextModule>::Cast(restoredModule), visitedModules);
    }
}
} // namespace

void ModuleDeregister::FreeModuleRecord([[maybe_unused]] void *env, void *pointer, void *hint)
{
    // LCOV_EXCL_BR_START
    if (pointer == nullptr) {
        LOG_FULL(FATAL) << "Lacking deregister module's name.";
        return;
    }
    // LCOV_EXCL_BR_STOP
    auto thread = reinterpret_cast<JSThread* >(hint);
    ThreadManagedScope managedScope(thread);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // pointer is module's name, which will be deregistered.
    JSTaggedValue moduleVal =
        thread->GetModuleManager()->HostGetImportedModule(pointer);
    NativeAreaAllocator* allocator = thread->GetEcmaVM()->GetNativeAreaAllocator();
    allocator->FreeBuffer(pointer);
    if (moduleVal.IsUndefined()) {
        return;
    }

    JSHandle<SourceTextModule> module(thread, SourceTextModule::Cast(moduleVal.GetTaggedObject()));
    LoadingTypes type = module->GetLoadingTypes();
    CString recordNameStr = SourceTextModule::GetModuleName(module.GetTaggedValue());
    if (type != LoadingTypes::DYNAMITC_MODULE) {
        LOG_FULL(DEBUG) << "free stable module's ModuleNameSpace" << recordNameStr;
        return;
    }

    std::set<CString> decreaseModule = {recordNameStr};
    DecreaseRegisterCounts(thread, module, decreaseModule);
    uint16_t counts = module->GetRegisterCounts();
    if (counts == 0) {
        thread->GetEcmaVM()->RemoveFromDeregisterModuleList(recordNameStr);
    }
    LOG_FULL(DEBUG) << "try to remove module " << recordNameStr << ", register counts is " << counts;
}

void ModuleDeregister::RemoveModule(JSThread *thread, JSHandle<SourceTextModule> module)
{
    CString recordName = SourceTextModule::GetModuleName(module.GetTaggedValue());
    thread->GetModuleManager()->RemoveModuleFromCacheToPending(recordName);
}

void ModuleDeregister::ResetRegisterCountsForDeregister(JSThread *thread, JSHandle<SourceTextModule> module)
{
    CUnorderedSet<CString, CStringHash> visitedModules;
    ResetRegisterCountsForDeregisterInner(thread, module, visitedModules);
}

void ModuleDeregister::RestoreModuleFromPending(JSThread *thread, const JSHandle<JSTaggedValue> &moduleRecord,
                                                const ExecuteTypes &executeType)
{
    InitForDeregisterModule(moduleRecord, executeType);
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    ResetRegisterCountsForDeregister(thread, module);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    if (executeType != ExecuteTypes::DYNAMIC) {
        SetModuleLoadingTypeToStable(thread, module);
    }
}

void ModuleDeregister::DecreaseRegisterCounts(JSThread *thread, JSHandle<SourceTextModule> module,
    std::set<CString> &decreaseModule)
{
    if (!module->GetRequestedModules(thread).IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules(thread));
        size_t requestedModulesLen = requestedModules->GetLength();
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            JSHandle<SourceTextModule> requiredModule =
                SourceTextModule::GetModuleFromCacheOrResolveNewOne(thread, module, requestedModules, idx);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            ASSERT(requiredModule.GetTaggedValue().IsSourceTextModule());
            CString moduleName = SourceTextModule::GetModuleName(requiredModule.GetTaggedValue());
            if (moduleName.empty()) {
                continue;
            }
            if (thread->GetModuleManager()->IsPendingRemovalModule(moduleName)) {
                continue;
            }
            if (decreaseModule.find(moduleName) != decreaseModule.end()) {
                continue;
            }
            decreaseModule.emplace(moduleName);
            // if module is lazy, this module and it's request module shoule not deregistered.
            // because module execute may contain in PromiseJob.
            if (requiredModule->GetStatus() < ModuleStatus::EVALUATED) {
                SetModuleLoadingTypeToStable(thread, requiredModule);
                continue;
            }
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
    // LCOV_EXCL_BR_START
    if (num == 0) {
        LOG_FULL(FATAL) << "moduleNameSpace can not be uninstalled";
    }
    // LCOV_EXCL_BR_STOP

    uint16_t registerNum = num - 1;
    if (registerNum == 0) {
        LOG_FULL(DEBUG) << "try to remove module " << SourceTextModule::GetModuleName(module.GetTaggedValue());
        RemoveModule(thread, module);
    }
    module->SetRegisterCounts(registerNum);
}

bool ModuleDeregister::TryToRemoveSO(JSThread *thread, JSHandle<SourceTextModule> module)
{
    UnloadNativeModuleCallback unloadNativeModuleCallback = thread->GetEcmaVM()->GetUnloadNativeModuleCallback();
    // LCOV_EXCL_BR_START
    if (unloadNativeModuleCallback == nullptr) {
        LOG_ECMA(ERROR) << "unloadNativeModuleCallback is nullptr";
        return false;
    }
    // LCOV_EXCL_BR_STOP

    CString soName = base::PathHelper::GetStrippedModuleName(module->GetEcmaModuleRecordNameString());
    return unloadNativeModuleCallback(soName.c_str());
}

void ModuleDeregister::SetModuleLoadingTypeToStable(JSThread *thread, JSHandle<SourceTextModule> module)
{
    if (!SourceTextModule::IsDynamicModule(module)) {
        return;
    }
    module->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
    if (!module->GetRequestedModules(thread).IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules(thread));
        size_t requestedModulesLen = requestedModules->GetLength();
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            JSHandle<SourceTextModule> requiredModule =
                SourceTextModule::GetModuleFromCacheOrResolveNewOne(thread, module, requestedModules, idx);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            ASSERT(requiredModule.GetTaggedValue().IsSourceTextModule());
            SetModuleLoadingTypeToStable(thread, requiredModule);
        }
    }
}
} // namespace panda::ecmascript
