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

#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/module/module_manager_helper.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/runtime_lock.h"

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
    for (auto &it : resolvedSharedModules_) {
        ObjectSlot slot(reinterpret_cast<uintptr_t>(&it.second));
        v(Root::ROOT_VM, slot);
        ASSERT(slot.GetTaggedValue() == it.second);
    }
}

JSTaggedValue SharedModuleManager::GetSendableModuleValue(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetSendableModuleValueImpl(thread, index, currentModule);
}

JSTaggedValue SharedModuleManager::GetSendableModuleValueImpl(
    JSThread *thread, int32_t index, JSTaggedValue currentModule) const
{
    if (currentModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
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
    if (currentModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
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
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
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
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << fileName;
    }
    JSRecordInfo *recordInfo = nullptr;
    [[maybe_unused]] bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(fileName, &recordInfo);
    ASSERT(hasRecord && !jsPandaFile->IsSharedModule(recordInfo));
    // loading unshared module though current context's module manager
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    return moduleManager->HostResolveImportedModule(fileName, executeFromJob);
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
    // before resolving module completely, shared-module put into isolate -thread resolvedModules_ temporarily.
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> module = moduleManager->TryGetImportedModule(recordName);
    if (!module->IsUndefined()) {
        return module;
    }

    ASSERT(jsPandaFile->IsModule(recordInfo));
    JSHandle<JSTaggedValue> moduleRecord = SharedModuleHelper::ParseSharedModule(thread, jsPandaFile, recordName,
                                                                                 fileName, recordInfo);
    JSHandle<SourceTextModule>::Cast(moduleRecord)->SetEcmaModuleRecordNameString(recordName);
    moduleManager->AddResolveImportedModule(recordName, moduleRecord.GetTaggedValue());
    moduleManager->AddToInstantiatingSModuleList(recordName);
    return moduleRecord;
}

bool SharedModuleManager::SearchInSModuleManagerUnsafe(const CString &recordName)
{
    auto entry = resolvedSharedModules_.find(recordName);
    if (entry != resolvedSharedModules_.end()) {
        return true;
    }
    return false;
}

JSHandle<SourceTextModule> SharedModuleManager::GetSModuleUnsafe(JSThread *thread, const CString &recordName)
{
    auto entry = resolvedSharedModules_.find(recordName);
    if (entry == resolvedSharedModules_.end()) {
        return JSHandle<SourceTextModule>::Cast(thread->GlobalConstants()->GetHandledUndefined());
    }
    JSHandle<JSTaggedValue> module(thread, entry->second);
    return JSHandle<SourceTextModule>::Cast(module);
}

JSHandle<SourceTextModule> SharedModuleManager::GetSModule(JSThread *thread, const CString &recordName)
{
    RuntimeLockHolder locker(thread, mutex_);
    return GetSModuleUnsafe(thread, recordName);
}

bool SharedModuleManager::SearchInSModuleManager(JSThread *thread, const CString &recordName)
{
    RuntimeLockHolder locker(thread, mutex_);
    return SearchInSModuleManagerUnsafe(recordName);
}
void SharedModuleManager::InsertInSModuleManager(JSThread *thread, const CString &recordName,
    JSHandle<SourceTextModule> &moduleRecord)
{
    RuntimeLockHolder locker(thread, mutex_);
    JSHandle<JSTaggedValue> module = JSHandle<JSTaggedValue>::Cast(moduleRecord);
    if (!SearchInSModuleManagerUnsafe(recordName)) {
        AddResolveImportedSModule(recordName, module.GetTaggedValue());
        StateVisit stateVisit;
        sharedModuleMutex_.emplace(recordName, std::move(stateVisit));
    }
}

void SharedModuleManager::TransferSModule(JSThread *thread)
{
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CVector<CString> instantiatingSModuleList = moduleManager->GetInstantiatingSModuleList();
    for (auto s:instantiatingSModuleList) {
        JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule(s);
        ASSERT(module->GetSharedType() == SharedTypes::SHARED_MODULE);
        InsertInSModuleManager(thread, s, module);
        moduleManager->RemoveModuleNameFromList(s);
    }
    moduleManager->ClearInstantiatingSModuleList();
}

StateVisit &SharedModuleManager::findModuleMutexWithLock(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    RuntimeLockHolder locker(thread, mutex_);
    CString moduleName = SourceTextModule::GetModuleName(module.GetTaggedValue());
    auto it = sharedModuleMutex_.find(moduleName);
    if (it == sharedModuleMutex_.end()) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << " Get shared module mutex failed";
    }
    return it->second;
}

bool SharedModuleManager::IsInstantiatedSModule(JSThread *thread, const JSHandle<SourceTextModule> &module)
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
    [[maybe_unused]] bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(recordName, &recordInfo);
    ASSERT(hasRecord);
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

void SharedModuleManager::SharedNativeObjDestory()
{
    for (auto it = resolvedSharedModules_.begin(); it != resolvedSharedModules_.end(); it++) {
        CString key = it->first;
        ASSERT(!key.empty());
        JSTaggedValue module = it->second;
        SourceTextModule::Cast(module)->DestoryLazyImportArray();
        SourceTextModule::Cast(module)->DestoryEcmaModuleFilenameString();
        SourceTextModule::Cast(module)->DestoryEcmaModuleRecordNameString();
    }
}
} // namespace panda::ecmascript
