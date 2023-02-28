/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/jspandafile/js_pandafile_executor.h"

#include "ecmascript/base/path_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/patch/quick_fix_manager.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;
Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromFile(JSThread *thread, const CString &filename,
    std::string_view entryPoint, bool needUpdate, bool excuteFromJob)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteFromFile filename " << filename;
    CString entry;
    CString name;
    CString normalName = PathHelper::NormalizePath(filename);
    EcmaVM *vm = thread->GetEcmaVM();
    if (!vm->IsBundlePack()) {
#if defined(PANDA_TARGET_LINUX) || defined(OHOS_UNIT_TEST)
        name = filename;
        entry = entryPoint.data();
#else
        if (excuteFromJob) {
            entry = entryPoint.data();
        } else {
            entry = PathHelper::ParseOhmUrl(vm, normalName, name);
        }
#if !WIN_OR_MAC_OR_IOS_PLATFORM
        if (name.empty()) {
            name = vm->GetAssetPath();
        }
#elif defined(PANDA_TARGET_WINDOWS)
        CString assetPath = vm->GetAssetPath();
        name = assetPath + "\\" + JSPandaFile::MERGE_ABC_NAME;
#else
        CString assetPath = vm->GetAssetPath();
        name = assetPath + "/" + JSPandaFile::MERGE_ABC_NAME;
#endif
#endif
    } else {
        name = filename;
        entry = entryPoint.data();
    }
 
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, name, entry.c_str(), needUpdate);
    if (jsPandaFile == nullptr) {
        CString msg = "Load file with filename '" + name + "' failed";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
    }
    if (!jsPandaFile->IsBundlePack() && !excuteFromJob && !vm->GetBundleName().empty()) {
        const_cast<JSPandaFile *>(jsPandaFile)->CheckIsNewRecord(vm);
        if (!jsPandaFile->IsNewRecord()) {
            PathHelper::CroppingRecord(entry);
        }
    }
    bool isModule = jsPandaFile->IsModule(thread, entry.c_str());
    if (thread->HasPendingException()) {
        vm->HandleUncaughtException(thread->GetException());
        return JSTaggedValue::Undefined();
    }
    if (isModule) {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        ModuleManager *moduleManager = vm->GetModuleManager();
        JSHandle<JSTaggedValue> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
        if (jsPandaFile->IsBundlePack()) {
            moduleRecord = moduleManager->HostResolveImportedModule(name);
        } else {
            moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(name, entry.c_str());
        }
        SourceTextModule::Instantiate(thread, moduleRecord);
        if (thread->HasPendingException()) {
            if (!excuteFromJob) {
                vm->HandleUncaughtException(thread->GetException());
            }
            return JSTaggedValue::Undefined();
        }
        JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
        module->SetStatus(ModuleStatus::INSTANTIATED);
        SourceTextModule::Evaluate(thread, module, nullptr, 0, excuteFromJob);
        return JSTaggedValue::Undefined();
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile, entry.c_str(), excuteFromJob);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromBuffer(JSThread *thread,
    const void *buffer, size_t size, std::string_view entryPoint, const CString &filename, bool needUpdate)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteFromBuffer filename " << filename;
    CString normalName = PathHelper::NormalizePath(filename);
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, normalName, entryPoint, buffer, size, needUpdate);
    if (jsPandaFile == nullptr) {
        CString msg = "Load file with filename '" + normalName + "' failed";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
    }

    CString entry = entryPoint.data();
    bool isModule = jsPandaFile->IsModule(thread, entry);
    bool isBundle = jsPandaFile->IsBundlePack();
    if (isModule) {
        return CommonExecuteBuffer(thread, isBundle, normalName, entry, buffer, size);
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile, entry);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteModuleBuffer(
    JSThread *thread, const void *buffer, size_t size, const CString &filename)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteModuleBuffer filename " << filename;
    CString name;
    EcmaVM *vm = thread->GetEcmaVM();
#if !WIN_OR_MAC_OR_IOS_PLATFORM
    name = vm->GetAssetPath();
#elif defined(PANDA_TARGET_WINDOWS)
    CString assetPath = vm->GetAssetPath();
    name = assetPath + "\\" + JSPandaFile::MERGE_ABC_NAME;
#else
    CString assetPath = vm->GetAssetPath();
    name = assetPath + "/" + JSPandaFile::MERGE_ABC_NAME;
#endif
    CString normalName = PathHelper::NormalizePath(filename);
    CString entry = PathHelper::ParseOhmUrl(vm, normalName, name);
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, name, entry.c_str(), buffer, size);
    if (jsPandaFile == nullptr) {
        if (thread->GetEcmaVM()->IsWorkerThread()) {
            CString mesWorker = "Excute worker's entryPoint failed: " + entry +
                ". Please check worker's reference path .";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, mesWorker.c_str(), Unexpected(false));
        }
        CString message = "Excute file's entryPoint failed: " + entry +
            ". Please check the reference path .";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, message.c_str(), Unexpected(false));
    }
    ASSERT(jsPandaFile->IsModule(thread, entry.c_str()));
    bool isBundle = jsPandaFile->IsBundlePack();
    if (!isBundle) {
        const_cast<JSPandaFile *>(jsPandaFile)->CheckIsNewRecord(vm);
        if (!jsPandaFile->IsNewRecord()) {
            PathHelper::CroppingRecord(entry);
        }
    }
    ASSERT(jsPandaFile->IsModule(thread, entry.c_str()));
    return CommonExecuteBuffer(thread, isBundle, name, entry, buffer, size);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::CommonExecuteBuffer(JSThread *thread,
    bool isBundle, const CString &filename, const CString &entry, const void *buffer, size_t size)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *vm = thread->GetEcmaVM();
    ModuleManager *moduleManager = vm->GetModuleManager();
    moduleManager->SetExecuteMode(true);
    JSHandle<JSTaggedValue> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
    if (isBundle) {
        moduleRecord = moduleManager->HostResolveImportedModule(buffer, size, filename);
    } else {
        moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(filename, entry);
    }
    SourceTextModule::Instantiate(thread, moduleRecord);
    if (thread->HasPendingException()) {
        vm->HandleUncaughtException(thread->GetException());
        return JSTaggedValue::Undefined();
    }
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    module->SetStatus(ModuleStatus::INSTANTIATED);
    SourceTextModule::Evaluate(thread, module, buffer, size);
    return JSTaggedValue::Undefined();
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::Execute(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                           std::string_view entryPoint, bool excuteFromJob)
{
    // For Ark application startup
    EcmaVM *vm = thread->GetEcmaVM();
    Expected<JSTaggedValue, bool> result = vm->InvokeEcmaEntrypoint(jsPandaFile, entryPoint, excuteFromJob);
    if (result) {
        QuickFixManager *quickFixManager = vm->GetQuickFixManager();
        quickFixManager->LoadPatchIfNeeded(thread, jsPandaFile);
    }
    return result;
}
}  // namespace panda::ecmascript
