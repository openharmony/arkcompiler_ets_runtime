/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "ecmascript/module/js_module_manager.h"

#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/module_resolver.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/require/js_cjs_module.h"

namespace panda::ecmascript {
using StringHelper = base::StringHelper;
using JSPandaFile = ecmascript::JSPandaFile;
using JSRecordInfo = ecmascript::JSPandaFile::JSRecordInfo;

ModuleManager::ModuleManager(EcmaVM *vm) : vm_(vm) {}

JSHandle<JSTaggedValue> ModuleManager::GenerateSendableFuncModule(const JSHandle<JSTaggedValue> &module)
{
    // Clone isolate module at shared-heap to mark sendable class.
    return SendableClassModule::GenerateSendableFuncModule(vm_->GetJSThread(), module);
}

bool ModuleManager::CheckModuleValueOutterResolved(JSThread *thread, int32_t index, JSFunction *jsFunc)
{
    // check module resolved, if resolved, load var from module directly for jit compiled code.
    ASSERT(jsFunc != nullptr);
    JSTaggedValue currentModule = jsFunc->GetModule(thread);
    if (!currentModule.IsSourceTextModule()) {
        return false;
    }
    if (SourceTextModule::IsSendableFunctionModule(currentModule)) {
        return false;
    }
    JSTaggedValue moduleEnv =
        reinterpret_cast<SourceTextModule*>(currentModule.GetTaggedObject())->GetEnvironment(thread);
    if (!moduleEnv.IsTaggedArray()) {
        return false;
    }
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnv.GetTaggedObject())->Get(thread, index);
    if (!resolvedBinding.IsResolvedIndexBinding()) {
        return false;
    }

    ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
    SourceTextModule *resolvedModule = reinterpret_cast<SourceTextModule*>(
        binding->GetModule(thread).GetTaggedObject());
    ModuleTypes moduleType = resolvedModule->GetTypes();
    if (moduleType != ModuleTypes::ECMA_MODULE) {
        return false;
    }

    JSTaggedValue resolvedModuleDict = resolvedModule->GetNameDictionary(thread);
    if (!resolvedModuleDict.IsTaggedArray()) {
        return false;
    }
    return true;
}

JSTaggedValue ModuleManager::GetExternalModuleVarFastPathForJIT(JSThread *thread, int32_t index, JSFunction *jsFunc)
{
    // fast path for jit load ecma module, check resolved in compiled
    // with CheckModuleValueOutterResolved, avoid redundancy check in runtime.
    ASSERT(jsFunc != nullptr);
    ASSERT(thread != nullptr);
    if (thread->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN) {
        return JSTaggedValue::Hole();
    }

    JSTaggedValue currentModule = jsFunc->GetModule(thread);
    ASSERT(currentModule.IsSourceTextModule());
    JSTaggedValue moduleEnvironment =
        reinterpret_cast<SourceTextModule*>(currentModule.GetTaggedObject())->GetEnvironment(thread);
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnvironment.GetTaggedObject())->Get(thread, index);
    ASSERT(resolvedBinding.IsResolvedIndexBinding());
    ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
    JSTaggedValue resolvedModule = binding->GetModule(thread);
    int32_t bindingIndex = binding->GetIndex();
    ASSERT(resolvedModule.IsSourceTextModule());
    JSTaggedValue dictionary =
        reinterpret_cast<SourceTextModule*>(resolvedModule.GetTaggedObject())->GetNameDictionary(thread);
    TaggedArray *array = TaggedArray::Cast(dictionary.GetTaggedObject());
    return array->Get(thread, bindingIndex);
}

JSHandle<SourceTextModule> ModuleManager::GetImportedModule(const CString &referencing)
{
    auto thread = vm_->GetJSThread();
    if (!IsLocalModuleLoaded(referencing)) {
        return SharedModuleManager::GetInstance()->GetSModule(thread, referencing);
    } else {
        return HostGetImportedModule(referencing);
    }
}

JSHandle<SourceTextModule> ModuleManager::HostGetImportedModule(const CString &referencing)
{
    auto entry = resolvedModules_.find(referencing);
    if (entry == resolvedModules_.end()) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Can not get module: " << referencing;
    }
    return JSHandle<SourceTextModule>(vm_->GetJSThread(), entry->second);
}

JSTaggedValue ModuleManager::HostGetImportedModule(void *src)
{
    const char *str = reinterpret_cast<char *>(src);
    CString referencing(str, strlen(str));
    LOG_FULL(INFO) << "current str during module deregister process : " << referencing;
    auto entry = resolvedModules_.find(referencing);
    if (entry == resolvedModules_.end()) {
        LOG_FULL(INFO) << "The module has been unloaded, " << referencing;
        return JSTaggedValue::Undefined();
    }
    JSTaggedValue result = entry->second;
    return result;
}

bool ModuleManager::IsLocalModuleLoaded(const CString& referencing)
{
    auto entry = resolvedModules_.find(referencing);
    if (entry == resolvedModules_.end()) {
        return false;
    }
    return true;
}

bool ModuleManager::IsSharedModuleLoaded(const CString &referencing)
{
    SharedModuleManager* sharedModuleManager = SharedModuleManager::GetInstance();
    return sharedModuleManager->SearchInSModuleManager(vm_->GetJSThread(), referencing);
}

bool ModuleManager::IsModuleLoaded(const CString &referencing)
{
    if (IsLocalModuleLoaded(referencing)) {
        return true;
    }
    SharedModuleManager* sharedModuleManager = SharedModuleManager::GetInstance();
    return sharedModuleManager->SearchInSModuleManager(vm_->GetJSThread(), referencing);
}

bool ModuleManager::IsEvaluatedModule(const CString &referencing)
{
    auto entry = resolvedModules_.find(referencing);
    if (entry == resolvedModules_.end()) {
        return false;
    }
    JSTaggedValue result = entry->second;
    // ModuleStatus == (EVALUATED || ERRORED).
    if (SourceTextModule::Cast(result.GetTaggedObject())->GetStatus() >= ModuleStatus::EVALUATED) {
        return true;
    }
    return false;
}

// This function assumes that referencing already existed in resolvedModules_/resolvedSharedModules_.
bool ModuleManager::IsInstantiatedModule(const CString &referencing)
{
    JSHandle<SourceTextModule> module = GetImportedModule(referencing);
    return module->GetStatus() == ModuleStatus::INSTANTIATED;
}

bool ModuleManager::IsLocalModuleInstantiated(const CString &referencing)
{
    JSHandle<SourceTextModule> module = HostGetImportedModule(referencing);
    return module->GetStatus() == ModuleStatus::INSTANTIATED;
}

bool ModuleManager::NeedExecuteModule(const CString &referencing)
{
    if (IsModuleLoaded(referencing)) {
        JSHandle<SourceTextModule> module = GetImportedModule(referencing);
        return module->GetStatus() == ModuleStatus::INSTANTIATED;
    }
    return true;
}

void ModuleManager::AddToInstantiatingSModuleList(const CString &record)
{
    InstantiatingSModuleList_.push_back(record);
}

CVector<CString> ModuleManager::GetInstantiatingSModuleList()
{
    return InstantiatingSModuleList_;
}

void ModuleManager::ClearInstantiatingSModuleList()
{
    InstantiatingSModuleList_.clear();
}

void ModuleManager::Iterate(RootVisitor &v)
{
    for (auto &it : resolvedModules_) {
        ObjectSlot slot(reinterpret_cast<uintptr_t>(&it.second));
        v.VisitRoot(Root::ROOT_VM, slot);
        ASSERT(slot.GetTaggedValue() == it.second);
    }
}

CString ModuleManager::GetRecordName(const JSThread *thread, JSTaggedValue module)
{
    CString entry = "";
    if (module.IsString()) {
        entry = ModulePathHelper::Utf8ConvertToString(const_cast<JSThread*>(thread), module);
    }
    if (module.IsSourceTextModule()) {
        SourceTextModule *sourceTextModule = SourceTextModule::Cast(module.GetTaggedObject());
        CString recordName = sourceTextModule->GetEcmaModuleRecordNameString();
        if (!recordName.empty()) {
            return recordName;
        }
    }
    return entry;
}

int ModuleManager::GetExportObjectIndex(EcmaVM *vm, JSHandle<SourceTextModule> ecmaModule,
                                        const CString &key)
{
    JSThread *thread = vm->GetJSThread();
    if (ecmaModule->GetLocalExportEntries(thread).IsUndefined()) {
        CString msg = "No export named '" + key;
        if (!ecmaModule->GetEcmaModuleRecordNameString().empty()) {
            msg += "' which exported by '" + ecmaModule->GetEcmaModuleRecordNameString() + "'";
        } else {
            msg += "' which exported by '" + ecmaModule->GetEcmaModuleFilenameString() + "'";
        }
        ObjectFactory *factory = vm->GetFactory();
        JSTaggedValue error = factory->GetJSError(ErrorType::SYNTAX_ERROR, msg.c_str(),
                                                  StackCheck::NO).GetTaggedValue();
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, 0);
    }
    JSHandle<TaggedArray> localExportEntries(thread, ecmaModule->GetLocalExportEntries(thread));
    size_t exportEntriesLen = localExportEntries->GetLength();
    // 0: There's only one export value "default"
    int index = 0;
    JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    if (exportEntriesLen > 1) { // 1:  The number of export objects exceeds 1
        for (size_t idx = 0; idx < exportEntriesLen; idx++) {
            ee.Update(localExportEntries->Get(thread, idx));
            if (EcmaStringAccessor(ee->GetExportName(thread)).Utf8ConvertToString(thread) == key) {
                ASSERT(idx <= static_cast<size_t>(INT_MAX));
                index = static_cast<int>(ee->GetLocalIndex());
                break;
            }
        }
    }
    return index;
}

JSHandle<JSTaggedValue> ModuleManager::LoadNativeModule(JSThread *thread, const CString &key)
{
    JSHandle<SourceTextModule> ecmaModule = JSHandle<SourceTextModule>::Cast(ExecuteNativeModule(thread, key));
    ASSERT(ecmaModule->GetIsNewBcVersion());
    int index = GetExportObjectIndex(vm_, ecmaModule, key);
    JSTaggedValue result = ecmaModule->GetModuleValue(thread, index, false);
    return JSHandle<JSTaggedValue>(thread, result);
}

JSHandle<JSTaggedValue> ModuleManager::ExecuteNativeModuleMayThrowError(JSThread *thread, const CString &recordName)
{
    JSMutableHandle<JSTaggedValue> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
    if (IsEvaluatedModule(recordName)) {
        JSHandle<SourceTextModule> moduleRecord = HostGetImportedModule(recordName);
        return JSHandle<JSTaggedValue>(thread, moduleRecord->GetModuleValue(thread, 0, false));
    }

    ModuleTypes moduleType = SourceTextModule::GetNativeModuleType(recordName);
    JSHandle<JSTaggedValue> moduleRecord = ModuleDataExtractor::ParseNativeModule(thread,
        recordName, "", moduleType);
    JSHandle<SourceTextModule> nativeModule =
        JSHandle<SourceTextModule>::Cast(moduleRecord);
    auto exportObject = SourceTextModule::LoadNativeModuleMayThrowError(thread, nativeModule, moduleType);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread,
        JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    SourceTextModule::RecordEvaluatedOrError(thread, nativeModule);
    nativeModule->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
    SourceTextModule::StoreModuleValue(thread, nativeModule, 0, exportObject);
    AddResolveImportedModule(recordName, moduleRecord.GetTaggedValue());
    return exportObject;
}

JSHandle<JSTaggedValue> ModuleManager::ExecuteNativeModule(JSThread *thread, const CString &recordName)
{
    JSMutableHandle<JSTaggedValue> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
    if (IsEvaluatedModule(recordName)) {
        JSHandle<SourceTextModule> moduleRecord = HostGetImportedModule(recordName);
        requiredModule.Update(moduleRecord);
    } else if (IsLocalModuleLoaded(recordName)) {
        JSHandle<SourceTextModule> nativeModule = HostGetImportedModule(recordName);
        SourceTextModule::EvaluateNativeModule(thread, nativeModule, nativeModule->GetTypes());
        nativeModule->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
        requiredModule.Update(nativeModule);
    } else {
        ModuleTypes moduleType = SourceTextModule::GetNativeModuleType(recordName);
        JSHandle<JSTaggedValue> nativeModuleHandle =
            ModuleResolver::ResolveNativeModule(thread, recordName, "", moduleType);
        JSHandle<SourceTextModule> nativeModule =
            JSHandle<SourceTextModule>::Cast(nativeModuleHandle);
        SourceTextModule::EvaluateNativeModule(thread, nativeModule, moduleType);
        nativeModule->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
        requiredModule.Update(nativeModule);
    }
    AddResolveImportedModule(recordName, requiredModule.GetTaggedValue());
    return requiredModule;
}

JSHandle<JSTaggedValue> ModuleManager::ExecuteJsonModule(JSThread *thread, const CString &recordName,
    const CString &filename, const JSPandaFile *jsPandaFile)
{
    JSMutableHandle<JSTaggedValue> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
    if (IsEvaluatedModule(recordName)) {
        JSHandle<SourceTextModule> moduleRecord = HostGetImportedModule(recordName);
        requiredModule.Update(moduleRecord);
    } else {
        JSHandle<SourceTextModule> moduleRecord =
            JSHandle<SourceTextModule>::Cast(ModuleDataExtractor::ParseJsonModule(thread, jsPandaFile, filename,
                                                                                  recordName));
        SourceTextModule::RecordEvaluatedOrError(thread, moduleRecord);
        requiredModule.Update(moduleRecord);
        UpdateResolveImportedModule(recordName, moduleRecord.GetTaggedValue());
    }
    return requiredModule;
}

JSHandle<JSTaggedValue> ModuleManager::ExecuteCjsModule(JSThread *thread, const CString &recordName,
    const JSPandaFile *jsPandaFile)
{
    CString entryPoint = JSPandaFile::ENTRY_FUNCTION_NAME;
    CString moduleRecord = jsPandaFile->GetJSPandaFileDesc();
    if (!jsPandaFile->IsBundlePack()) {
        entryPoint = recordName;
        moduleRecord = recordName;
    }

    JSMutableHandle<JSTaggedValue> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
    if (IsEvaluatedModule(moduleRecord)) {
        requiredModule.Update(HostGetImportedModule(moduleRecord));
    } else {
        JSHandle<SourceTextModule> module =
            JSHandle<SourceTextModule>::Cast(ModuleDataExtractor::ParseCjsModule(thread, jsPandaFile));
        module->SetEcmaModuleRecordNameString(moduleRecord);
        requiredModule.Update(module);
        AddResolveImportedModule(recordName, module.GetTaggedValue());
        JSPandaFileExecutor::Execute(thread, jsPandaFile, entryPoint);
        SourceTextModule::RecordEvaluatedOrError(thread, JSHandle<SourceTextModule>::Cast(requiredModule));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, requiredModule);
        UpdateResolveImportedModule(recordName, module.GetTaggedValue());
    }
    return requiredModule;
}

JSHandle<JSTaggedValue> ModuleManager::TryGetImportedModule(const CString& referencing)
{
    JSThread *thread = vm_->GetJSThread();
    auto entry = resolvedModules_.find(referencing);
    if (entry == resolvedModules_.end()) {
        return thread->GlobalConstants()->GetHandledUndefined();
    }
    return JSHandle<JSTaggedValue>(thread, entry->second);
}

void ModuleManager::RemoveModuleFromCache(const CString& recordName)
{
    auto entry = resolvedModules_.find(recordName);
    if (entry == resolvedModules_.end()) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Can not get module: " << recordName <<
            ", when try to remove the module";
    }
    JSTaggedValue result = entry->second;
    SourceTextModule::Cast(result)->DestoryLazyImportArray();
    SourceTextModule::Cast(result)->DestoryEcmaModuleFilenameString();
    SourceTextModule::Cast(result)->DestoryEcmaModuleRecordNameString();
    resolvedModules_.erase(recordName);
}

// this function only remove module's name from resolvedModules List, it's content still needed by sharedmodule
void ModuleManager::RemoveModuleNameFromList(const CString& recordName)
{
    auto entry = resolvedModules_.find(recordName);
    if (entry == resolvedModules_.end()) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Can not get module: " << recordName <<
            ", when try to remove the module";
    }
    resolvedModules_.erase(recordName);
}

JSTaggedValue ModuleManager::CreateModuleManagerNativePointer(JSThread *thread)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ModuleManager *moduleManager = new ModuleManager(vm);
    moduleManager->SyncModuleExecuteMode(thread);
    auto factory = vm->GetFactory();
    JSHandle<JSNativePointer> nativePointer = factory->NewJSNativePointer(reinterpret_cast<void *>(moduleManager),
                                                                          nullptr, nullptr, true);
    vm->AddModuleManager(moduleManager);
    return nativePointer.GetTaggedValue();
}

void ModuleManager::SyncModuleExecuteMode(JSThread *thread)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetGlobalEnv();
    if (!globalEnv.GetTaggedValue().IsHole()) {
        JSHandle<JSNativePointer> nativePointer(globalEnv->GetModuleManagerNativePointer());
        ModuleManager *moduleManager = reinterpret_cast<ModuleManager *>(nativePointer->GetExternalPointer());
        if (moduleManager == nullptr) {
            return;
        }
        SetExecuteMode(moduleManager->GetExecuteMode());
    }
}
} // namespace panda::ecmascript
