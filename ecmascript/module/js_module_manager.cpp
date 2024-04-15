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

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_array.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_manager_helper.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/tagged_dictionary.h"
#ifdef PANDA_TARGET_WINDOWS
#include <algorithm>
#endif

namespace panda::ecmascript {
using StringHelper = base::StringHelper;
using JSPandaFile = ecmascript::JSPandaFile;
using JSRecordInfo = ecmascript::JSPandaFile::JSRecordInfo;

ModuleManager::ModuleManager(EcmaVM *vm) : vm_(vm)
{
    resolvedModules_ = NameDictionary::Create(vm_->GetJSThread(), DEAULT_DICTIONART_CAPACITY).GetTaggedValue();
}

JSTaggedValue ModuleManager::GetCurrentModule()
{
    FrameHandler frameHandler(vm_->GetJSThread());
    JSTaggedValue currentFunc = frameHandler.GetFunction();
    JSTaggedValue module = JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule();
    if (SourceTextModule::IsSendableFunctionModule(module)) {
        JSTaggedValue recordName = SourceTextModule::GetModuleName(module);
        return HostGetImportedModule(recordName).GetTaggedValue();
    }
    return module;
}

JSHandle<JSTaggedValue> ModuleManager::GenerateSendableFuncModule(const JSHandle<JSTaggedValue> &module)
{
    // Clone isolate module at shared-heap to mark sendable class.
    return SendableClassModule::GenerateSendableFuncModule(vm_->GetJSThread(), module);
}

JSTaggedValue ModuleManager::GetModuleValueInner(int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), index, false);
}

JSTaggedValue ModuleManager::GetModuleValueInner(int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), index, false);
}

JSTaggedValue ModuleManager::GetModuleValueInner(int32_t index, JSHandle<JSTaggedValue> currentModule)
{
    if (currentModule->IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule->GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), index, false);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule();
    return GetModuleValueOutterInternal(index, currentModule);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetModuleValueOutterInternal(index, currentModule);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(int32_t index, JSHandle<JSTaggedValue> currentModule)
{
    return GetModuleValueOutterInternal(index, currentModule.GetTaggedValue());
}

JSTaggedValue ModuleManager::GetModuleValueOutterInternal(int32_t index, JSTaggedValue currentModule)
{
    JSThread *thread = vm_->GetJSThread();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueOutter currentModule failed";
        UNREACHABLE();
    }
    JSHandle<SourceTextModule> currentModuleHdl(thread, currentModule);
    JSTaggedValue moduleEnvironment = currentModuleHdl->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnvironment.GetTaggedObject())->Get(index);
    if (resolvedBinding.IsResolvedIndexBinding()) {
        ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
        JSTaggedValue resolvedModule = binding->GetModule();
        JSHandle<SourceTextModule> module(thread, resolvedModule);
        ASSERT(resolvedModule.IsSourceTextModule());

        // Support for only modifying var value of HotReload.
        // Cause patchFile exclude the record of importing modifying var. Can't reresolve moduleRecord.
        EcmaContext *context = thread->GetCurrentEcmaContext();
        if (context->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN) {
            const JSHandle<JSTaggedValue> resolvedModuleOfHotReload =
                context->FindPatchModule(ConvertToString(module->GetEcmaModuleRecordName()));
            if (!resolvedModuleOfHotReload->IsHole()) {
                resolvedModule = resolvedModuleOfHotReload.GetTaggedValue();
            }
        }
        return ModuleManagerHelper::GetModuleValue(thread, module, binding->GetIndex());
    }
    if (resolvedBinding.IsResolvedBinding()) {
        ResolvedBinding *binding = ResolvedBinding::Cast(resolvedBinding.GetTaggedObject());
        JSTaggedValue resolvedModule = binding->GetModule();
        JSHandle<SourceTextModule> module(thread, resolvedModule);
        if (SourceTextModule::IsNativeModule(module->GetTypes())) {
            return ModuleManagerHelper::GetNativeModuleValue(thread, resolvedModule, binding);
        }
        if (module->GetTypes() == ModuleTypes::CJS_MODULE) {
            JSHandle<JSTaggedValue> cjsModuleName(thread, SourceTextModule::GetModuleName(module.GetTaggedValue()));
            return CjsModule::SearchFromModuleCache(thread, cjsModuleName).GetTaggedValue();
        }
    }
    if (resolvedBinding.IsResolvedRecordBinding()) {
        return ModuleManagerHelper::GetModuleValueFromRecordBinding(thread, currentModuleHdl, resolvedBinding);
    }
    LOG_ECMA(FATAL) << "Get module value failed, mistaken ResolvedBinding";
    UNREACHABLE();
}

void ModuleManager::StoreModuleValue(int32_t index, JSTaggedValue value)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, GetCurrentModule());
    StoreModuleValueInternal(currentModule, index, value);
}

void ModuleManager::StoreModuleValue(int32_t index, JSTaggedValue value, JSTaggedValue jsFunc)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule());
    StoreModuleValueInternal(currentModule, index, value);
}

void ModuleManager::StoreModuleValueInternal(JSHandle<SourceTextModule> &currentModule,
                                             int32_t index, JSTaggedValue value)
{
    if (currentModule.GetTaggedValue().IsUndefined()) {
        LOG_FULL(FATAL) << "StoreModuleValue currentModule failed";
        UNREACHABLE();
    }
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    currentModule->StoreModuleValue(thread, index, valueHandle);
}

JSTaggedValue ModuleManager::GetModuleValueInner(JSTaggedValue key)
{
    JSTaggedValue currentModule = GetCurrentModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
        UNREACHABLE();
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), key, false);
}

JSTaggedValue ModuleManager::GetModuleValueInner(JSTaggedValue key, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
        UNREACHABLE();
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), key, false);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(JSTaggedValue key)
{
    JSTaggedValue currentModule = GetCurrentModule();
    return GetModuleValueOutterInternal(key, currentModule);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(JSTaggedValue key, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetModuleValueOutterInternal(key, currentModule);
}

JSTaggedValue ModuleManager::GetModuleValueOutterInternal(JSTaggedValue key, JSTaggedValue currentModule)
{
    JSThread *thread = vm_->GetJSThread();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueOutter currentModule failed";
        UNREACHABLE();
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(currentModule.GetTaggedObject())->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    int entry = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->FindEntry(key);
    if (entry == -1) {
        return thread->GlobalConstants()->GetUndefined();
    }
    JSTaggedValue resolvedBinding = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->GetValue(entry);
    ASSERT(resolvedBinding.IsResolvedBinding());
    ResolvedBinding *binding = ResolvedBinding::Cast(resolvedBinding.GetTaggedObject());
    JSTaggedValue resolvedModule = binding->GetModule();
    ASSERT(resolvedModule.IsSourceTextModule());
    SourceTextModule *module = SourceTextModule::Cast(resolvedModule.GetTaggedObject());
    if (module->GetTypes() == ModuleTypes::CJS_MODULE) {
        JSHandle<JSTaggedValue> cjsModuleName(thread, SourceTextModule::GetModuleName(JSTaggedValue(module)));
        return CjsModule::SearchFromModuleCache(thread, cjsModuleName).GetTaggedValue();
    }
    return module->GetModuleValue(thread, binding->GetBindingName(), false);
}

void ModuleManager::StoreModuleValue(JSTaggedValue key, JSTaggedValue value)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, GetCurrentModule());
    StoreModuleValueInternal(currentModule, key, value);
}

void ModuleManager::StoreModuleValue(JSTaggedValue key, JSTaggedValue value, JSTaggedValue jsFunc)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule());
    StoreModuleValueInternal(currentModule, key, value);
}

void ModuleManager::StoreModuleValueInternal(JSHandle<SourceTextModule> &currentModule,
                                             JSTaggedValue key, JSTaggedValue value)
{
    if (currentModule.GetTaggedValue().IsUndefined()) {
        LOG_FULL(FATAL) << "StoreModuleValue currentModule failed";
        UNREACHABLE();
    }
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    currentModule->StoreModuleValue(thread, keyHandle, valueHandle);
}

JSHandle<SourceTextModule> ModuleManager::HostGetImportedModule(const CString &referencingModule)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<EcmaString> referencingHandle = factory->NewFromUtf8(referencingModule);
    return HostGetImportedModule(referencingHandle.GetTaggedValue());
}

JSHandle<SourceTextModule> ModuleManager::HostGetImportedModule(JSTaggedValue referencing)
{
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencing);
    LOG_ECMA_IF(entry == -1, FATAL) << "Can not get module: "
                                    << ConvertToString(referencing);
    JSTaggedValue result = dict->GetValue(entry);
    return JSHandle<SourceTextModule>(vm_->GetJSThread(), result);
}

JSTaggedValue ModuleManager::HostGetImportedModule(void *src)
{
    const char *str = reinterpret_cast<char *>(src);
    const uint8_t *strData = reinterpret_cast<uint8_t *>(src);
    LOG_FULL(INFO) << "current str during module deregister process : " << str;
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(strData, strlen(str));
    if (entry == -1) {
        LOG_FULL(INFO) << "The module has been unloaded, " << str;
        return JSTaggedValue::Undefined();
    }
    JSTaggedValue result = dict->GetValue(entry);
    return result;
}

bool ModuleManager::IsImportedModuleLoaded(JSTaggedValue referencing)
{
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencing);
    if (entry == -1) {
        return false;
    }
    JSTaggedValue result = dict->GetValue(entry).GetWeakRawValue();
    dict->UpdateValue(vm_->GetJSThread(), entry, result);
    return true;
}

bool ModuleManager::IsEvaluatedModule(JSTaggedValue referencing)
{
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());

    int entry = dict->FindEntry(referencing);
    if (entry == -1) {
        return false;
    }
    JSTaggedValue result = dict->GetValue(entry);
    if (SourceTextModule::Cast(result.GetTaggedObject())->GetStatus() ==
        ModuleStatus::EVALUATED) {
            return true;
    }
    return false;
}

JSHandle<JSTaggedValue> ModuleManager::ResolveModuleInMergedABC(JSThread *thread, const JSPandaFile *jsPandaFile,
    const CString &recordName, bool executeFromJob)
{
    // In static parse Phase, due to lack of some parameters, we will create a empty SourceTextModule which will
    // be marked as INSTANTIATED to skip Dfs traversal of this import branch.
    if (!vm_->EnableReportModuleResolvingFailure() && (jsPandaFile == nullptr ||
        (jsPandaFile != nullptr && !jsPandaFile->HasRecord(recordName)))) {
        return CreateEmptyModule();
    } else {
        return ResolveModuleWithMerge(thread, jsPandaFile, recordName, executeFromJob);
    }
}

JSHandle<JSTaggedValue> ModuleManager::HostResolveImportedModuleWithMerge(const CString &moduleFileName,
    const CString &recordName, bool executeFromJob)
{
    JSHandle<EcmaString> recordNameHandle = vm_->GetFactory()->NewFromUtf8(recordName);
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(recordNameHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<JSTaggedValue>(vm_->GetJSThread(), dict->GetValue(entry));
    }
    return CommonResolveImportedModuleWithMerge(moduleFileName, recordName, executeFromJob);
}

JSHandle<JSTaggedValue> ModuleManager::HostResolveImportedModuleWithMergeForHotReload(const CString &moduleFileName,
    const CString &recordName, bool executeFromJob)
{
    return CommonResolveImportedModuleWithMerge(moduleFileName, recordName, executeFromJob);
}

JSHandle<JSTaggedValue> ModuleManager::CommonResolveImportedModuleWithMerge(const CString &moduleFileName,
    const CString &recordName, bool executeFromJob)
{
    JSThread *thread = vm_->GetJSThread();

    std::shared_ptr<JSPandaFile> jsPandaFile = ModulePathHelper::SkipDefaultBundleFile(thread, moduleFileName) ?
        nullptr : JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, recordName, false);
    if (jsPandaFile == nullptr) {
        // In Aot Module Instantiate, we miss some runtime parameters from framework like bundleName or moduleName
        // which may cause wrong recordName parsing and we also can't load files not in this app hap. But in static
        // phase, these should not be an error, just skip it is ok.
        if (vm_->EnableReportModuleResolvingFailure()) {
            LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << moduleFileName;
        }
    }
    JSHandle<JSTaggedValue> moduleRecord = ResolveModuleInMergedABC(thread,
        jsPandaFile.get(), recordName, executeFromJob);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    JSHandle<NameDictionary> handleDict(thread, resolvedModules_);
    JSHandle<EcmaString> recordNameHandle= vm_->GetFactory()->NewFromUtf8(recordName);
    resolvedModules_ = NameDictionary::Put(thread, handleDict, JSHandle<JSTaggedValue>(recordNameHandle),
        moduleRecord, PropertyAttributes::Default()).GetTaggedValue();

    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleManager::CreateEmptyModule()
{
    if (!cachedEmptyModule_.IsHole()) {
        return JSHandle<JSTaggedValue>(vm_->GetJSThread(), cachedEmptyModule_);
    }
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<SourceTextModule> tmpModuleRecord = factory->NewSourceTextModule();
    tmpModuleRecord->SetStatus(ModuleStatus::INSTANTIATED);
    tmpModuleRecord->SetTypes(ModuleTypes::ECMA_MODULE);
    tmpModuleRecord->SetIsNewBcVersion(true);
    cachedEmptyModule_ = tmpModuleRecord.GetTaggedValue();
    return JSHandle<JSTaggedValue>::Cast(tmpModuleRecord);
}

JSHandle<JSTaggedValue> ModuleManager::HostResolveImportedModule(const CString &referencingModule, bool executeFromJob)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<EcmaString> referencingHandle = factory->NewFromUtf8(referencingModule);
    CString moduleFileName = referencingModule;
    if (vm_->IsBundlePack()) {
        if (AOTFileManager::GetAbsolutePath(referencingModule, moduleFileName)) {
            referencingHandle = factory->NewFromUtf8(moduleFileName);
        } else {
            CString msg = "Parse absolute " + referencingModule + " path failed";
            THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
        }
    }

    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<JSTaggedValue>(thread, dict->GetValue(entry));
    }

    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (jsPandaFile == nullptr) {
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << moduleFileName;
    }

    return ResolveModule(thread, jsPandaFile.get(), executeFromJob);
}

// The security interface needs to be modified accordingly.
JSHandle<JSTaggedValue> ModuleManager::HostResolveImportedModule(const void *buffer, size_t size,
                                                                 const CString &filename)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<EcmaString> referencingHandle = factory->NewFromUtf8(filename);
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<JSTaggedValue>(thread, dict->GetValue(entry));
    }

    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, filename,
                                                           JSPandaFile::ENTRY_MAIN_FUNCTION, buffer, size);
    if (jsPandaFile == nullptr) {
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << filename;
    }

    return ResolveModule(thread, jsPandaFile.get());
}

JSHandle<JSTaggedValue> ModuleManager::ResolveModule(JSThread *thread, const JSPandaFile *jsPandaFile,
    bool executeFromJob)
{
    ObjectFactory *factory = vm_->GetFactory();
    CString moduleFileName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<JSTaggedValue> moduleRecord = thread->GlobalConstants()->GetHandledUndefined();
    JSRecordInfo recordInfo = const_cast<JSPandaFile *>(jsPandaFile)->FindRecordInfo(JSPandaFile::ENTRY_FUNCTION_NAME);
    if (jsPandaFile->IsModule(recordInfo)) {
        moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, moduleFileName, moduleFileName);
    } else {
        ASSERT(jsPandaFile->IsCjs(recordInfo));
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, jsPandaFile);
    }
    // json file can not be compiled into isolate abc.
    ASSERT(!jsPandaFile->IsJson(recordInfo));
    ModuleDeregister::InitForDeregisterModule(moduleRecord, executeFromJob);
    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    JSHandle<JSTaggedValue> referencingHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(moduleFileName));
    resolvedModules_ =
        NameDictionary::Put(thread, dict, referencingHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleManager::ResolveNativeModule(const CString &moduleRequestName,
    const CString &baseFileName, ModuleTypes moduleType)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSThread *thread = vm_->GetJSThread();

    JSHandle<JSTaggedValue> referencingModule(factory->NewFromUtf8(moduleRequestName));
    JSHandle<JSTaggedValue> moduleRecord = ModuleDataExtractor::ParseNativeModule(thread,
        moduleRequestName, baseFileName, moduleType);
    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    resolvedModules_ = NameDictionary::Put(thread, dict, referencingModule, moduleRecord,
        PropertyAttributes::Default()).GetTaggedValue();
    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleManager::ResolveModuleWithMerge(
    JSThread *thread, const JSPandaFile *jsPandaFile, const CString &recordName, bool executeFromJob)
{
    ObjectFactory *factory = vm_->GetFactory();
    CString moduleFileName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<JSTaggedValue> moduleRecord = thread->GlobalConstants()->GetHandledUndefined();
    JSRecordInfo recordInfo;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(recordName, recordInfo);
    if (!hasRecord) {
        JSHandle<JSTaggedValue> exp(thread, JSTaggedValue::Exception());
        THROW_MODULE_NOT_FOUND_ERROR_WITH_RETURN_VALUE(thread, recordName, moduleFileName, exp);
    }
    if (jsPandaFile->IsModule(recordInfo)) {
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, recordName, moduleFileName);
    } else if (jsPandaFile->IsJson(recordInfo)) {
        moduleRecord = ModuleDataExtractor::ParseJsonModule(thread, jsPandaFile, moduleFileName, recordName);
    } else {
        ASSERT(jsPandaFile->IsCjs(recordInfo));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, jsPandaFile);
    }

    JSHandle<JSTaggedValue> recordNameHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(recordName));
    JSHandle<SourceTextModule>::Cast(moduleRecord)->SetEcmaModuleRecordName(thread, recordNameHandle);
    ModuleDeregister::InitForDeregisterModule(moduleRecord, executeFromJob);
    return moduleRecord;
}

void ModuleManager::AddResolveImportedModule(JSHandle<JSTaggedValue> &record, JSHandle<JSTaggedValue> &module)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    resolvedModules_ =
        NameDictionary::Put(thread, dict, record, module, PropertyAttributes::Default())
        .GetTaggedValue();
}

void ModuleManager::AddResolveImportedModule(const CString &referencingModule, JSHandle<JSTaggedValue> moduleRecord)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> referencingHandle(factory->NewFromUtf8(referencingModule));
    AddResolveImportedModule(referencingHandle, moduleRecord);
}

void ModuleManager::AddToInstantiatingSModuleList(const CString &record)
{
    InstantiatingSModuleList_.push_back(record);
}

CVector<CString> ModuleManager::GetInstantiatingSModuleList()
{
    return InstantiatingSModuleList_;
}

JSTaggedValue ModuleManager::GetModuleNamespace(int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule();
    return GetModuleNamespaceInternal(index, currentModule);
}

JSTaggedValue ModuleManager::GetModuleNamespace(int32_t index, JSTaggedValue currentFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule();
    return GetModuleNamespaceInternal(index, currentModule);
}

JSTaggedValue ModuleManager::GetModuleNamespaceInternal(int32_t index, JSTaggedValue currentModule)
{
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleNamespace currentModule failed";
        UNREACHABLE();
    }
    JSThread *thread = vm_->GetJSThread();
    SourceTextModule *module = SourceTextModule::Cast(currentModule.GetTaggedObject());
    JSTaggedValue requestedModule = module->GetRequestedModules();
    JSTaggedValue moduleName = TaggedArray::Cast(requestedModule.GetTaggedObject())->Get(index);
    JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
    JSHandle<JSTaggedValue> requiredModule;
    if (moduleRecordName.IsUndefined()) {
        requiredModule = SourceTextModule::HostResolveImportedModule(thread,
            JSHandle<SourceTextModule>(thread, module), JSHandle<JSTaggedValue>(thread, moduleName));
    } else {
        ASSERT(moduleRecordName.IsString());
        requiredModule = SourceTextModule::HostResolveImportedModuleWithMerge(thread,
            JSHandle<SourceTextModule>(thread, module), JSHandle<JSTaggedValue>(thread, moduleName));
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    JSHandle<SourceTextModule> requiredModuleST = JSHandle<SourceTextModule>::Cast(requiredModule);
    ModuleTypes moduleType = requiredModuleST->GetTypes();
    // if requiredModuleST is Native module
    if (SourceTextModule::IsNativeModule(moduleType)) {
        return SourceTextModule::Cast(requiredModuleST.GetTaggedValue())->GetModuleValue(thread, 0, false);
    }
    // if requiredModuleST is CommonJS
    if (moduleType == ModuleTypes::CJS_MODULE) {
        JSHandle<JSTaggedValue> cjsModuleName(thread,
            SourceTextModule::GetModuleName(requiredModuleST.GetTaggedValue()));
        return CjsModule::SearchFromModuleCache(thread, cjsModuleName).GetTaggedValue();
    }
    // if requiredModuleST is ESM
    JSHandle<JSTaggedValue> moduleNamespace = SourceTextModule::GetModuleNamespace(thread, requiredModuleST);
    ASSERT(moduleNamespace->IsModuleNamespace());
    return moduleNamespace.GetTaggedValue();
}

JSTaggedValue ModuleManager::GetModuleNamespace(JSTaggedValue localName)
{
    JSTaggedValue currentModule = GetCurrentModule();
    return GetModuleNamespaceInternal(localName, currentModule);
}

JSTaggedValue ModuleManager::GetModuleNamespace(JSTaggedValue localName, JSTaggedValue currentFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule();
    return GetModuleNamespaceInternal(localName, currentModule);
}

JSTaggedValue ModuleManager::GetModuleNamespaceInternal(JSTaggedValue localName, JSTaggedValue currentModule)
{
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleNamespace currentModule failed";
        UNREACHABLE();
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(currentModule.GetTaggedObject())->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return vm_->GetJSThread()->GlobalConstants()->GetUndefined();
    }
    int entry = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->FindEntry(localName);
    if (entry == -1) {
        return vm_->GetJSThread()->GlobalConstants()->GetUndefined();
    }
    JSTaggedValue moduleNamespace = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->GetValue(entry);
    ASSERT(moduleNamespace.IsModuleNamespace());
    return moduleNamespace;
}

void ModuleManager::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&resolvedModules_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&cachedEmptyModule_)));
}

CString ModuleManager::GetRecordName(JSTaggedValue module)
{
    CString entry = "";
    if (module.IsString()) {
        entry = ConvertToString(module);
    }
    if (module.IsSourceTextModule()) {
        SourceTextModule *sourceTextModule = SourceTextModule::Cast(module.GetTaggedObject());
        if (sourceTextModule->GetEcmaModuleRecordName().IsString()) {
            entry = ConvertToString(sourceTextModule->GetEcmaModuleRecordName());
        }
    }
    return entry;
}

int ModuleManager::GetExportObjectIndex(EcmaVM *vm, JSHandle<SourceTextModule> ecmaModule,
                                        const std::string &key)
{
    JSThread *thread = vm->GetJSThread();
    if (ecmaModule->GetLocalExportEntries().IsUndefined()) {
        CString msg = "No export named '" + ConvertToString(key);
        if (!ecmaModule->GetEcmaModuleRecordName().IsUndefined()) {
            msg += "' which exported by '" + ConvertToString(ecmaModule->GetEcmaModuleRecordName()) + "'";
        } else {
            msg += "' which exported by '" + ConvertToString(ecmaModule->GetEcmaModuleFilename()) + "'";
        }
        ObjectFactory *factory = vm->GetFactory();
        JSTaggedValue error = factory->GetJSError(ErrorType::SYNTAX_ERROR, msg.c_str()).GetTaggedValue();
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, 0);
    }
    JSHandle<TaggedArray> localExportEntries(thread, ecmaModule->GetLocalExportEntries());
    size_t exportEntriesLen = localExportEntries->GetLength();
    // 0: There's only one export value "default"
    int index = 0;
    JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    if (exportEntriesLen > 1) { // 1:  The number of export objects exceeds 1
        for (size_t idx = 0; idx < exportEntriesLen; idx++) {
            ee.Update(localExportEntries->Get(idx));
            if (EcmaStringAccessor(ee->GetExportName()).ToStdString() == key) {
                ASSERT(idx <= static_cast<size_t>(INT_MAX));
                index = static_cast<int>(ee->GetLocalIndex());
                break;
            }
        }
    }
    return index;
}

JSHandle<JSTaggedValue> ModuleManager::HostResolveImportedModule(const JSPandaFile *jsPandaFile,
                                                                 const CString &filename)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<EcmaString> referencingHandle = vm_->GetFactory()->NewFromUtf8(filename);
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<JSTaggedValue>(thread, dict->GetValue(entry));
    }

    if (jsPandaFile == nullptr) {
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << filename;
    }
    return ResolveModule(thread, jsPandaFile);
}

JSHandle<JSTaggedValue> ModuleManager::LoadNativeModule(JSThread *thread, const std::string &key)
{
    JSHandle<SourceTextModule> ecmaModule = JSHandle<SourceTextModule>::Cast(ExecuteNativeModule(thread, key));
    ASSERT(ecmaModule->GetIsNewBcVersion());
    int index = GetExportObjectIndex(vm_, ecmaModule, key);
    JSTaggedValue result = ecmaModule->GetModuleValue(thread, index, false);
    return JSHandle<JSTaggedValue>(thread, result);
}

JSHandle<JSTaggedValue> ModuleManager::ExecuteNativeModule(JSThread *thread, const std::string &recordName)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<EcmaString> record = factory->NewFromASCII(recordName.c_str());
    JSMutableHandle<JSTaggedValue> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
    if (IsEvaluatedModule(record.GetTaggedValue())) {
        JSHandle<SourceTextModule> moduleRecord = HostGetImportedModule(record.GetTaggedValue());
        requiredModule.Update(moduleRecord);
    } else {
        CString requestPath = ConvertToString(record.GetTaggedValue());
        CString entryPoint = PathHelper::GetStrippedModuleName(requestPath);
        auto [isNative, moduleType] = SourceTextModule::CheckNativeModule(requestPath);
        JSHandle<JSTaggedValue> nativeModuleHandle = ResolveNativeModule(requestPath, "", moduleType);
        JSHandle<SourceTextModule> nativeModule =
            JSHandle<SourceTextModule>::Cast(nativeModuleHandle);
        if (!SourceTextModule::LoadNativeModule(thread, nativeModule, moduleType)) {
            LOG_FULL(ERROR) << "loading native module " << requestPath << " failed";
        }
        nativeModule->SetStatus(ModuleStatus::EVALUATED);
        nativeModule->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
        requiredModule.Update(nativeModule);
    }
    return requiredModule;
}

JSHandle<JSTaggedValue> ModuleManager::ExecuteJsonModule(JSThread *thread, const std::string &recordName,
    const CString &filename, const JSPandaFile *jsPandaFile)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<EcmaString> record = factory->NewFromASCII(recordName.c_str());
    JSMutableHandle<JSTaggedValue> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
    if (IsEvaluatedModule(record.GetTaggedValue())) {
        JSHandle<SourceTextModule> moduleRecord = HostGetImportedModule(record.GetTaggedValue());
        requiredModule.Update(moduleRecord);
    } else {
        JSHandle<SourceTextModule> moduleRecord =
            JSHandle<SourceTextModule>::Cast(ModuleDataExtractor::ParseJsonModule(thread, jsPandaFile, filename,
                                                                                  recordName.c_str()));
        moduleRecord->SetStatus(ModuleStatus::EVALUATED);
        requiredModule.Update(moduleRecord);
    }
    return requiredModule;
}

JSHandle<JSTaggedValue> ModuleManager::ExecuteCjsModule(JSThread *thread, const std::string &recordName,
    const JSPandaFile *jsPandaFile)
{
    ObjectFactory *factory = vm_->GetFactory();
    CString entryPoint = JSPandaFile::ENTRY_FUNCTION_NAME;
    CString moduleRecord = jsPandaFile->GetJSPandaFileDesc();
    if (!jsPandaFile->IsBundlePack()) {
        entryPoint = recordName;
        moduleRecord = recordName;
    }
    JSHandle<EcmaString> record = factory->NewFromASCII(moduleRecord);

    JSMutableHandle<JSTaggedValue> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
    if (IsEvaluatedModule(record.GetTaggedValue())) {
        requiredModule.Update(HostGetImportedModule(record.GetTaggedValue()));
    } else {
        JSHandle<SourceTextModule> module =
            JSHandle<SourceTextModule>::Cast(ModuleDataExtractor::ParseCjsModule(thread, jsPandaFile));
        module->SetEcmaModuleRecordName(thread, record);
        requiredModule.Update(module);
        JSPandaFileExecutor::Execute(thread, jsPandaFile, entryPoint);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, requiredModule);
        module->SetStatus(ModuleStatus::EVALUATED);
    }
    return requiredModule;
}

JSHandle<JSTaggedValue> ModuleManager::GetModuleNameSpaceFromFile(
    JSThread *thread, std::string &recordNameStr, std::string &baseFileName)
{
    JSHandle<EcmaString> recordName = thread->GetEcmaVM()->GetFactory()->NewFromASCII(recordNameStr.c_str());
    if (!IsImportedModuleLoaded(recordName.GetTaggedValue())) {
        if (!ecmascript::JSPandaFileExecutor::ExecuteFromAbcFile(
            thread, baseFileName.c_str(), recordNameStr.c_str(), false, true)) {
            LOG_ECMA(ERROR) << "LoadModuleNameSpaceFromFile:Cannot execute module: %{public}s, recordName: %{public}s",
                baseFileName.c_str(), recordNameStr.c_str();
            return thread->GlobalConstants()->GetHandledUndefinedString();
        }
    }
    JSHandle<ecmascript::SourceTextModule> moduleRecord = HostGetImportedModule(recordName.GetTaggedValue());
    moduleRecord->SetLoadingTypes(ecmascript::LoadingTypes::STABLE_MODULE);
    return ecmascript::SourceTextModule::GetModuleNamespace(
        thread, JSHandle<ecmascript::SourceTextModule>(moduleRecord));
}

JSHandle<JSTaggedValue> ModuleManager::TryGetImportedModule(JSTaggedValue referencing)
{
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    JSThread *thread = vm_->GetJSThread();
    int entry = dict->FindEntry(referencing);
    if (entry == -1) {
        return thread->GlobalConstants()->GetHandledUndefined();
    }
    JSTaggedValue result = dict->GetValue(entry);
    return JSHandle<JSTaggedValue>(thread, result);
}

void ModuleManager::RemoveModuleFromCache(JSTaggedValue recordName)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<NameDictionary> dict(thread, resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(recordName);
    LOG_ECMA_IF(entry == -1, FATAL) << "Can not get module: " << ConvertToString(recordName) <<
         ", when try to remove the module";

    resolvedModules_  = NameDictionary::Remove(thread, dict, entry).GetTaggedValue();
}
} // namespace panda::ecmascript
