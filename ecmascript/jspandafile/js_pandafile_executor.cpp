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
#include "ecmascript/module/js_module_manager.h"

namespace panda::ecmascript {
Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromFile(JSThread *thread, const CString &filename,
                                                                   std::string_view entryPoint)
{
    CString entry = entryPoint.data();
    CString name = filename;
#if ECMASCRIPT_ENABLE_MERGE_ABC
    if (!thread->GetEcmaVM()->IsBundle()) {
        entry = JSPandaFile::ParseOhmUrl(filename.c_str());
#if !WIN_OR_MAC_PLATFORM
        name = JSPandaFile::MERGE_ABC_PATH;
#elif defined(PANDA_TARGET_WINDOWS)
    CString assetPath = thread->GetEcmaVM()->GetAssetPath().c_str();
    name = assetPath + "\\modules.abc";
#else
    CString assetPath = thread->GetEcmaVM()->GetAssetPath().c_str();
    name = assetPath + "/modules.abc";
#endif
    }
#endif
    const JSPandaFile *jsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, name, entry.c_str());
    if (jsPandaFile == nullptr) {
        return Unexpected(false);
    }
    if (jsPandaFile->IsBundle()) {
        entry = JSPandaFile::ENTRY_FUNCTION_NAME;
    }
    bool isModule = jsPandaFile->IsModule(entry.c_str());
    if (isModule) {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        EcmaVM *vm = thread->GetEcmaVM();
        ModuleManager *moduleManager = vm->GetModuleManager();
        moduleManager->SetExecuteMode(false);
        JSHandle<SourceTextModule> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
        if (jsPandaFile->IsBundle()) {
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
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, filename, entryPoint, buffer, size);
    if (jsPandaFile == nullptr) {
        return Unexpected(false);
    }
    CString entry;
    if (jsPandaFile->IsBundle()) {
        entry = JSPandaFile::ENTRY_FUNCTION_NAME;
    } else {
        entry = entryPoint.data();
    }
    bool isModule = jsPandaFile->IsModule(entry.data());
    if (isModule) {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        EcmaVM *vm = thread->GetEcmaVM();
        ModuleManager *moduleManager = vm->GetModuleManager();
        moduleManager->SetExecuteMode(true);
        JSHandle<SourceTextModule> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
        if (jsPandaFile->IsBundle()) {
            moduleRecord = moduleManager->HostResolveImportedModule(buffer, size, filename);
        } else {
            moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(filename, entryPoint.data());
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
    return JSPandaFileExecutor::Execute(thread, jsPandaFile, entry.c_str());
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::Execute(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                           std::string_view entryPoint)
{
    // For Ark application startup
    EcmaVM *vm = thread->GetEcmaVM();
    Expected<JSTaggedValue, bool> result = vm->InvokeEcmaEntrypoint(jsPandaFile, entryPoint);
    return result;
}
}  // namespace panda::ecmascript
