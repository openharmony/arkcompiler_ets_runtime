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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/jspandafile/quick_fix_manager.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/module/js_module_manager.h"

namespace panda::ecmascript {
Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromFile(JSThread *thread, const CString &filename,
                                                                   std::string_view entryPoint)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteFromFile filename " << filename.c_str();

    CString entry = entryPoint.data();
    CString name = filename;
    if (!thread->GetEcmaVM()->IsBundlePack()) {
#if defined(PANDA_TARGET_LINUX)
        entry = JSPandaFile::ParseRecordName(filename);
#else
        entry = JSPandaFile::ParseOhmUrl(filename);
#if !WIN_OR_MAC_PLATFORM
        name = thread->GetEcmaVM()->GetAssetPath().c_str();
#elif defined(PANDA_TARGET_WINDOWS)
    CString assetPath = thread->GetEcmaVM()->GetAssetPath().c_str();
    name = assetPath + "\\modules.abc";
#else
    CString assetPath = thread->GetEcmaVM()->GetAssetPath().c_str();
    name = assetPath + "/modules.abc";
#endif
#endif
    }

    const JSPandaFile *jsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, name, entry.c_str());
    if (jsPandaFile == nullptr) {
        return Unexpected(false);
    }
    bool isModule = jsPandaFile->IsModule(entry.c_str());
    if (isModule) {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        EcmaVM *vm = thread->GetEcmaVM();
        ModuleManager *moduleManager = vm->GetModuleManager();
        moduleManager->SetExecuteMode(false);
        JSHandle<SourceTextModule> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
        if (jsPandaFile->IsBundlePack()) {
            moduleRecord = moduleManager->HostResolveImportedModule(name);
        } else {
            moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(name, entry.c_str());
        }
        SourceTextModule::Instantiate(thread, moduleRecord);
        if (thread->HasPendingException()) {
            auto exception = thread->GetException();
            vm->HandleUncaughtException(exception.GetTaggedObject());
            return JSTaggedValue::Undefined();
        }
        SourceTextModule::Evaluate(thread, moduleRecord);
        return JSTaggedValue::Undefined();
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile, entry.c_str());
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromBuffer(
    JSThread *thread, const void *buffer, size_t size, std::string_view entryPoint, const CString &filename)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteFromBuffer filename " << filename.c_str();
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, filename, entryPoint, buffer, size);
    if (jsPandaFile == nullptr) {
        return Unexpected(false);
    }
    CString entry = entryPoint.data();
    bool isModule = jsPandaFile->IsModule(entry);
    bool isBundle = jsPandaFile->IsBundlePack();
    if (isModule) {
        return CommonExecuteBuffer(thread, isBundle, filename, entry, buffer, size);
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile, entry);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteModuleBuffer(
    JSThread *thread, const void *buffer, size_t size, const CString &filename)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteModuleBuffer filename " << filename.c_str();
    CString name;
#if !WIN_OR_MAC_PLATFORM
    name = thread->GetEcmaVM()->GetAssetPath().c_str();
#elif defined(PANDA_TARGET_WINDOWS)
    CString assetPath = thread->GetEcmaVM()->GetAssetPath().c_str();
    name = assetPath + "\\modules.abc";
#else
    CString assetPath = thread->GetEcmaVM()->GetAssetPath().c_str();
    name = assetPath + "/modules.abc";
#endif
    CString entry = JSPandaFile::ParseOhmUrl(filename);
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, name, entry.c_str(), buffer, size);
    if (jsPandaFile == nullptr) {
        return Unexpected(false);
    }
    ASSERT(jsPandaFile->IsModule(entry.c_str()));
    bool isBundle = jsPandaFile->IsBundlePack();
    return CommonExecuteBuffer(thread, isBundle, name, entry, buffer, size);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::CommonExecuteBuffer(JSThread *thread,
    bool isBundle, const CString &filename, const CString &entry, const void *buffer, size_t size)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *vm = thread->GetEcmaVM();
    ModuleManager *moduleManager = vm->GetModuleManager();
    moduleManager->SetExecuteMode(true);
    JSHandle<SourceTextModule> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
    if (isBundle) {
        moduleRecord = moduleManager->HostResolveImportedModule(buffer, size, filename);
    } else {
        moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(filename, entry);
    }
    SourceTextModule::Instantiate(thread, moduleRecord);
    if (thread->HasPendingException()) {
        auto exception = thread->GetException();
        vm->HandleUncaughtException(exception.GetTaggedObject());
        return JSTaggedValue::Undefined();
    }
    SourceTextModule::Evaluate(thread, moduleRecord, buffer, size);
    return JSTaggedValue::Undefined();
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::Execute(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                           std::string_view entryPoint)
{
    // For Ark application startup
    EcmaVM *vm = thread->GetEcmaVM();
    Expected<JSTaggedValue, bool> result = vm->InvokeEcmaEntrypoint(jsPandaFile, entryPoint);
    if (result) {
        QuickFixManager *quickFixManager = vm->GetQuickFixManager();
        quickFixManager->LoadPatchIfNeeded(thread, CstringConvertToStdString(jsPandaFile->GetJSPandaFileDesc()));
    }
    return result;
}
}  // namespace panda::ecmascript
