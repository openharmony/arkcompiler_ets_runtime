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

void SharedModuleManager::Iterate(RootVisitor &v)
{
    for (auto &it : resolvedSharedModules_) {
        ObjectSlot slot(reinterpret_cast<uintptr_t>(&it.second));
        v.VisitRoot(Root::ROOT_VM, slot);
        ASSERT(slot.GetTaggedValue() == it.second);
    }
}

JSTaggedValue SharedModuleManager::GetSendableModuleValueInner(JSThread* thread, int32_t index, JSTaggedValue jsFunc)
{
    ModuleManager* moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetSendableModuleValueInner currentModule is undefined";
    }
    if (!SourceTextModule::IsSendableFunctionModule(currentModule)) {
        return SourceTextModule::Cast(currentModule)->GetModuleValue(thread, index, false);
    }
    CString referenceName = SourceTextModule::GetModuleName(currentModule);
    if (moduleManager->IsModuleLoaded(referenceName)) {
        JSHandle<SourceTextModule> currentModuleHdl = moduleManager->GetImportedModule(referenceName);
        if (currentModuleHdl->GetStatus() > ModuleStatus::INSTANTIATED) {
            return currentModuleHdl->GetModuleValue(thread, index, false);
        }
    }
    JSHandle<SourceTextModule> currentModuleHdl(thread, currentModule);
    auto isMergedAbc = !currentModuleHdl->GetEcmaModuleRecordNameString().empty();
    CString fileName = currentModuleHdl->GetEcmaModuleFilenameString();
    if (!JSPandaFileExecutor::LazyExecuteModule(thread, referenceName, fileName, isMergedAbc)) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "GetSendableModuleValueInner LazyExecuteModule failed";
    }
    ASSERT(moduleManager->IsModuleLoaded(referenceName));
    return moduleManager->GetImportedModule(referenceName)->GetModuleValue(thread, index, false);
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
        // LATER DO: cause assert failed
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
    JSRecordInfo *recordInfo = jsPandaFile->CheckAndGetRecordInfo(recordName);
    ASSERT(recordInfo != nullptr);
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
