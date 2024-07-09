/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ecmascript/module/js_shared_module_manager.h"

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_manager_helper.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/runtime_lock.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
using StringHelper = base::StringHelper;
using JSPandaFile = ecmascript::JSPandaFile;
using JSRecordInfo = ecmascript::JSPandaFile::JSRecordInfo;

SharedModuleManager* SharedModuleManager::GetInstance()
{
    static SharedModuleManager* instance = new SharedModuleManager();
    return instance;
}

void SharedModuleManager::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&resolvedSharedModules_)));
}

JSTaggedValue SharedModuleManager::GetSendableModuleValue(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetSendableModuleValueImpl(thread, index, currentModule);
}

JSTaggedValue SharedModuleManager::GetSendableModuleValueImpl(
    JSThread *thread, int32_t index, JSTaggedValue currentModule) const
{
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueOutter currentModule failed";
        UNREACHABLE();
    }

    JSHandle<SourceTextModule> module(thread, currentModule.GetTaggedObject());
    JSTaggedValue moduleEnvironment = module->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnvironment.GetTaggedObject())->Get(index);
    if (resolvedBinding.IsResolvedRecordIndexBinding()) {
        return ModuleManagerHelper::GetModuleValueFromIndexBinding(thread, module, resolvedBinding);
    } else if (resolvedBinding.IsResolvedIndexBinding()) {
        ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
        JSHandle<SourceTextModule> resolvedModule(thread, binding->GetModule().GetTaggedObject());
        return ModuleManagerHelper::GetModuleValue(thread, resolvedModule, binding->GetIndex());
    } else if (resolvedBinding.IsResolvedRecordBinding()) {
        return ModuleManagerHelper::GetModuleValueFromRecordBinding(thread, module, resolvedBinding);
    }
    LOG_ECMA(FATAL) << "Unexpect binding";
    UNREACHABLE();
}

JSTaggedValue SharedModuleManager::GetLazySendableModuleValue(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetLazySendableModuleValueImpl(thread, index, currentModule);
}

JSTaggedValue SharedModuleManager::GetLazySendableModuleValueImpl(
    JSThread *thread, int32_t index, JSTaggedValue currentModule) const
{
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueOutter currentModule failed";
        UNREACHABLE();
    }

    JSHandle<SourceTextModule> module(thread, currentModule.GetTaggedObject());
    JSTaggedValue moduleEnvironment = module->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnvironment.GetTaggedObject())->Get(index);
    if (resolvedBinding.IsResolvedRecordIndexBinding()) {
        return ModuleManagerHelper::GetLazyModuleValueFromIndexBinding(thread, module, resolvedBinding);
    } else if (resolvedBinding.IsResolvedIndexBinding()) {
        ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
        JSHandle<SourceTextModule> resolvedModule(thread, binding->GetModule().GetTaggedObject());
        SourceTextModule::Evaluate(thread, resolvedModule, nullptr);
        if (thread->HasPendingException()) {
            return JSTaggedValue::Undefined();
        }
        return ModuleManagerHelper::GetModuleValue(thread, resolvedModule, binding->GetIndex());
    } else if (resolvedBinding.IsResolvedRecordBinding()) {
        return ModuleManagerHelper::GetLazyModuleValueFromRecordBinding(thread, module, resolvedBinding);
    }
    LOG_ECMA(FATAL) << "Unexpect binding";
    UNREACHABLE();
}

JSHandle<JSTaggedValue> SharedModuleManager::ResolveImportedModule(JSThread *thread, const CString &fileName,
                                                                   bool executeFromJob)
{
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, fileName, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (jsPandaFile == nullptr) {
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << fileName;
    }
    JSRecordInfo *recordInfo = nullptr;
    jsPandaFile->CheckAndGetRecordInfo(fileName, &recordInfo);
    if (jsPandaFile->IsSharedModule(recordInfo)) {
        return ResolveSharedImportedModule(thread, fileName, jsPandaFile.get(), recordInfo);
    }
    // loading unshared module though current context's module manager
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    return moduleManager->HostResolveImportedModule(fileName, executeFromJob);
}

JSHandle<JSTaggedValue> SharedModuleManager::ResolveSharedImportedModule(JSThread *thread,
    const CString &referencingModule, const JSPandaFile *jsPandaFile, [[maybe_unused]] JSRecordInfo *recordInfo)
{
    if (SearchInSModuleManager(thread, referencingModule)) {
        return JSHandle<JSTaggedValue>(GetSModule(thread, referencingModule));
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> requireModule;
    CString fileName = referencingModule;
    if (AOTFileManager::GetAbsolutePath(referencingModule, fileName)) {
        requireModule = JSHandle<JSTaggedValue>(factory->NewFromUtf8(fileName));
    } else {
        CString msg = "Parse absolute shared module" + referencingModule + " path failed";
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }

    // before resolving module completely, shared-module put into isolate -thread resolvedModules_ temporarily.
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> module = moduleManager->TryGetImportedModule(requireModule.GetTaggedValue());
    if (!module->IsUndefined()) {
        return module;
    }

    ASSERT(jsPandaFile->IsModule(recordInfo));
    JSHandle<JSTaggedValue> moduleRecord = SharedModuleHelper::ParseSharedModule(thread,
        jsPandaFile, fileName, fileName, recordInfo);
    moduleManager->AddResolveImportedModule(requireModule, moduleRecord);
    moduleManager->AddToInstantiatingSModuleList(fileName);
    return moduleRecord;
}

JSHandle<JSTaggedValue> SharedModuleManager::ResolveImportedModuleWithMerge(JSThread *thread,
    const CString &fileName, const CString &recordName, bool executeFromJob)
{
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, fileName, recordName, false);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    if (jsPandaFile == nullptr) {
        CString msg = "Load file with filename '" + fileName + "' failed, recordName '" + recordName + "'";
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(recordName, &recordInfo);
    if (!hasRecord) {
        CString msg = "cannot find record '" + recordName + "', please check the request path.'"
                      + fileName + "'.";
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }

    if (jsPandaFile->IsSharedModule(recordInfo)) {
        return ResolveSharedImportedModuleWithMerge(thread, fileName, recordName, jsPandaFile.get(),
                                                    recordInfo);
    }
    // loading unshared module though current context's module manager
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    return moduleManager->HostResolveImportedModuleWithMerge(fileName, recordName, executeFromJob);
}

JSHandle<JSTaggedValue> SharedModuleManager::ResolveSharedImportedModuleWithMerge(JSThread *thread,
    const CString &fileName, const CString &recordName, const JSPandaFile *jsPandaFile,
    [[maybe_unused]] JSRecordInfo *recordInfo)
{
    if (SearchInSModuleManager(thread, recordName)) {
        return JSHandle<JSTaggedValue>(GetSModule(thread, recordName));
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> requireModule = JSHandle<JSTaggedValue>(factory->NewFromUtf8(recordName));
    // before resolving module completely, shared-module put into isolate -thread resolvedModules_ temporarily.
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> module = moduleManager->TryGetImportedModule(requireModule.GetTaggedValue());
    if (!module->IsUndefined()) {
        return module;
    }

    ASSERT(jsPandaFile->IsModule(recordInfo));
    JSHandle<JSTaggedValue> moduleRecord = SharedModuleHelper::ParseSharedModule(thread, jsPandaFile, recordName,
                                                                                 fileName, recordInfo);
    JSHandle<SourceTextModule>::Cast(moduleRecord)->SetEcmaModuleRecordName(thread, requireModule);
    moduleManager->AddResolveImportedModule(requireModule, moduleRecord);
    moduleManager->AddToInstantiatingSModuleList(recordName);
    return moduleRecord;
}

bool SharedModuleManager::SearchInSModuleManagerUnsafe(JSThread *thread, const CString &recordName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> recordNameHandle(JSHandle<JSTaggedValue>(factory->NewFromUtf8(recordName)));
    NameDictionary *dict = NameDictionary::Cast(resolvedSharedModules_.GetTaggedObject());
    int entry = dict->FindEntry(recordNameHandle.GetTaggedValue());
    if (entry != -1) {
        return true;
    }
    return false;
}

JSHandle<SourceTextModule> SharedModuleManager::GetSModuleUnsafe(JSThread *thread, const CString &recordName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> recordNameHandle(JSHandle<JSTaggedValue>(factory->NewFromUtf8(recordName)));
    NameDictionary *dict = NameDictionary::Cast(resolvedSharedModules_.GetTaggedObject());
    int entry = dict->FindEntry(recordNameHandle.GetTaggedValue());
    ASSERT(entry != -1);
    return JSHandle<SourceTextModule>(thread, dict->GetValue(entry));
}

JSHandle<SourceTextModule> SharedModuleManager::GetSModuleUnsafe(JSThread *thread, JSTaggedValue recordName)
{
    NameDictionary *dict = NameDictionary::Cast(resolvedSharedModules_.GetTaggedObject());
    int entry = dict->FindEntry(recordName);
    ASSERT(entry != -1);
    return JSHandle<SourceTextModule>(thread, dict->GetValue(entry));
}

JSHandle<SourceTextModule> SharedModuleManager::GetSModule(JSThread *thread, const CString &recordName)
{
    RuntimeLockHolder locker(thread, mutex_);
    return GetSModuleUnsafe(thread, recordName);
}

JSHandle<SourceTextModule> SharedModuleManager::GetSModule(JSThread *thread, JSTaggedValue recordName)
{
    RuntimeLockHolder locker(thread, mutex_);
    return GetSModuleUnsafe(thread, recordName);
}

bool SharedModuleManager::SearchInSModuleManager(JSThread *thread, const CString &recordName)
{
    RuntimeLockHolder locker(thread, mutex_);
    return SearchInSModuleManagerUnsafe(thread, recordName);
}
void SharedModuleManager::InsertInSModuleManager(JSThread *thread, JSHandle<JSTaggedValue> requireModule,
    JSHandle<SourceTextModule> &moduleRecord)
{
    RuntimeLockHolder locker(thread, mutex_);
    JSHandle<JSTaggedValue> module = JSHandle<JSTaggedValue>::Cast(moduleRecord);
    CString recordName = ModulePathHelper::Utf8ConvertToString(requireModule.GetTaggedValue());
    if (!SearchInSModuleManagerUnsafe(thread, recordName)) {
        JSHandle<NameDictionary> handleDict(thread, resolvedSharedModules_);
        resolvedSharedModules_ =
            NameDictionary::Put(thread, handleDict, requireModule, module, PropertyAttributes::Default())
            .GetTaggedValue();
        StateVisit stateVisit;
        sharedModuleMutex_.emplace(recordName, stateVisit);
    }
}

void SharedModuleManager::TransferSModule(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CVector<CString> instantiatingSModuleList = moduleManager->GetInstantiatingSModuleList();
    for (auto s:instantiatingSModuleList) {
        JSHandle<JSTaggedValue> requireModule = JSHandle<JSTaggedValue>(factory->NewFromUtf8(s));
        JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule(requireModule.GetTaggedValue());
        ASSERT(module->GetStatus() == ModuleStatus::INSTANTIATED &&
            module->GetSharedType() == SharedTypes::SHARED_MODULE);
        InsertInSModuleManager(thread, requireModule, module);
        moduleManager->RemoveModuleFromCache(requireModule.GetTaggedValue());
    }
    moduleManager->ClearInstantiatingSModuleList();
}

StateVisit &SharedModuleManager::findModuleMutexWithLock(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    RuntimeLockHolder locker(thread, mutex_);
    CString moduleName =
        ModulePathHelper::Utf8ConvertToString(SourceTextModule::GetModuleName(module.GetTaggedValue()));
    auto it = sharedModuleMutex_.find(moduleName);
    if (it == sharedModuleMutex_.end()) {
        LOG_ECMA(FATAL) << " Get shared module mutex failed";
    }
    return it->second;
}

bool SharedModuleManager::IsInstaniatedSModule(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    RuntimeLockHolder locker(thread, mutex_);
    return (module->GetStatus() >= ModuleStatus::INSTANTIATED);
}

JSHandle<JSTaggedValue> SharedModuleManager::GenerateFuncModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                                const CString &entryPoint, ClassKind classKind)
{
    CString recordName = jsPandaFile->GetRecordName(entryPoint);
    auto vm = thread->GetEcmaVM();
    JSRecordInfo *recordInfo = nullptr;
    jsPandaFile->CheckAndGetRecordInfo(recordName, &recordInfo);
    if (jsPandaFile->IsModule(recordInfo)) {
        JSHandle<SourceTextModule> module;
        if (jsPandaFile->IsSharedModule(recordInfo)) {
            return JSHandle<JSTaggedValue>(GetSModule(thread, entryPoint));
        } else {
            ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
            module = moduleManager->HostGetImportedModule(recordName);
        }

        if (classKind == ClassKind::NON_SENDABLE) {
            return JSHandle<JSTaggedValue>(module);
        } else {
            // Clone isolate module at shared-heap to mark sendable class.
            return SendableClassModule::GenerateSendableFuncModule(thread, JSHandle<JSTaggedValue>(module));
        }
    }
    return JSHandle<JSTaggedValue>(vm->GetFactory()->NewFromUtf8(recordName));
}

JSHandle<ModuleNamespace> SharedModuleManager::SModuleNamespaceCreate(JSThread *thread,
    const JSHandle<JSTaggedValue> &module, const JSHandle<TaggedArray> &exports)
{
    RuntimeLockHolder locker(thread, mutex_);
    return JSSharedModule::SModuleNamespaceCreate(thread, module, exports);
}
} // namespace panda::ecmascript
