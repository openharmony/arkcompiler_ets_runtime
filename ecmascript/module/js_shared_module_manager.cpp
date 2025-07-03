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
    resolvedSharedModules_.ForEach(
        [&v](auto iter) { iter->second.VisitRoot([&v](ObjectSlot slot) { v.VisitRoot(Root::ROOT_VM, slot); }); });
}

bool SharedModuleManager::SearchInSModuleManagerUnsafe(const CString &recordName)
{
    auto entry = resolvedSharedModules_.Find(recordName);
    if (entry) {
        return true;
    }
    return false;
}

JSHandle<SourceTextModule> SharedModuleManager::GetSModuleUnsafe(JSThread *thread, const CString &recordName)
{
    auto entry = resolvedSharedModules_.Find(recordName);
    if (!entry) {
        return JSHandle<SourceTextModule>(thread->GlobalConstants()->GetHandledUndefined());
    }
    JSHandle<JSTaggedValue> module(thread, entry.value());
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

bool SharedModuleManager::TryInsertInSModuleManager(JSThread *thread, const CString &recordName,
                                                    const JSHandle<SourceTextModule> &moduleRecord)
{
    RuntimeLockHolder locker(thread, mutex_);
    JSHandle<JSTaggedValue> module = JSHandle<JSTaggedValue>::Cast(moduleRecord);
    return AddResolveImportedSModule(recordName, module.GetTaggedValue());
}

void SharedModuleManager::AddToResolvedModulesAndCreateSharedModuleMutex(
    JSThread *thread, const CString &recordName, JSTaggedValue module)
{
    // Add to normal module resolvedModules_
    thread->GetModuleManager()->AddResolveImportedModule(recordName, module);
    RuntimeLockHolder locker(thread, mutex_);
    StateVisit stateVisit;
    sharedModuleMutex_.emplace(recordName, std::move(stateVisit));
}

JSHandle<SourceTextModule> SharedModuleManager::TransferFromLocalToSharedModuleMapAndGetInsertedSModule(
    JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    ASSERT(module->GetSharedType() == SharedTypes::SHARED_MODULE);
    CString recordName = module->GetEcmaModuleRecordNameString();
    bool success = TryInsertInSModuleManager(thread, recordName, module);
    thread->GetModuleManager()->RemoveModuleNameFromList(recordName);
    return success? module : GetSModule(thread, recordName);
}

StateVisit &SharedModuleManager::FindModuleMutexWithLock(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    RuntimeLockHolder locker(thread, mutex_);
    CString moduleName = SourceTextModule::GetModuleName(module.GetTaggedValue());
    auto it = sharedModuleMutex_.find(moduleName);
    if (it == sharedModuleMutex_.end()) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << " Get shared module mutex failed, moduleName is " << moduleName;
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
            ModuleManager *moduleManager = thread->GetModuleManager();
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
    resolvedSharedModules_.ForEach([](auto it) {
        CString key = it->first;
        ASSERT(!key.empty());
        GCRoot &root = it->second;
        JSTaggedValue module = root.Read();
        SourceTextModule::Cast(module)->DestoryLazyImportArray();
        SourceTextModule::Cast(module)->DestoryEcmaModuleFilenameString();
        SourceTextModule::Cast(module)->DestoryEcmaModuleRecordNameString();
    });
}
} // namespace panda::ecmascript
