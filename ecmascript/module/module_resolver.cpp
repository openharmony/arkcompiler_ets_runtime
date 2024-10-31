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

#include "ecmascript/module/module_resolver.h"

#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/module/module_logger.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/runtime_lock.h"
#include "ecmascript/patch/quick_fix_manager.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> ModuleResolver::HostResolveImportedModule(JSThread *thread,
                                                                  const JSHandle<SourceTextModule> &module,
                                                                  const JSHandle<JSTaggedValue> &moduleRequest,
                                                                  bool executeFromJob)
{
    return module->GetEcmaModuleRecordNameString().empty() ?
        HostResolveImportedModuleBundlePack(thread, module, moduleRequest, executeFromJob) :
        HostResolveImportedModuleWithMerge(thread, module, moduleRequest, executeFromJob);
}
// new way with module
JSHandle<JSTaggedValue> ModuleResolver::HostResolveImportedModuleWithMerge(JSThread *thread,
                                                                           const JSHandle<SourceTextModule> &module,
                                                                           const JSHandle<JSTaggedValue> &moduleRequest,
                                                                           bool executeFromJob)
{
    CString moduleRequestName = ModulePathHelper::Utf8ConvertToString(moduleRequest.GetTaggedValue());
    CString requestStr = ReplaceModuleThroughFeature(thread, moduleRequestName);

    CString baseFilename{};
    StageOfHotReload stageOfHotReload = thread->GetCurrentEcmaContext()->GetStageOfHotReload();
    if (stageOfHotReload == StageOfHotReload::BEGIN_EXECUTE_PATCHMAIN ||
        stageOfHotReload == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN) {
        baseFilename = thread->GetEcmaVM()->GetQuickFixManager()->GetBaseFileName(module);
    } else {
        baseFilename = module->GetEcmaModuleFilenameString();
    }

    auto moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    auto [isNative, moduleType] = SourceTextModule::CheckNativeModule(requestStr);
    if (isNative) {
        if (moduleManager->IsLocalModuleLoaded(requestStr)) {
            return JSHandle<JSTaggedValue>(moduleManager->HostGetImportedModule(requestStr));
        }
        return ResolveNativeModule(thread, requestStr, baseFilename, moduleType);
    }
    CString recordName = module->GetEcmaModuleRecordNameString();
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFilename, recordName);
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << baseFilename;
    }

    CString outFileName = baseFilename;
    CString entryPoint =
        ModulePathHelper::ConcatFileNameWithMerge(thread, jsPandaFile.get(), outFileName, recordName, requestStr);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

#if defined(PANDA_TARGET_WINDOWS) || defined(PANDA_TARGET_MACOS)
    if (entryPoint == ModulePathHelper::PREVIEW_OF_ACROSS_HAP_FLAG) {
        THROW_SYNTAX_ERROR_AND_RETURN(thread, "", thread->GlobalConstants()->GetHandledUndefined());
    }
#endif
    return ResolveImportedModuleWithMerge(thread, outFileName, entryPoint, executeFromJob);
}

// old way with bundle
JSHandle<JSTaggedValue> ModuleResolver::HostResolveImportedModuleBundlePack(JSThread *thread,
    const JSHandle<SourceTextModule> &module,
    const JSHandle<JSTaggedValue> &moduleRequest,
    bool executeFromJob)
{
    auto moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CString moduleRequestStr = ModulePathHelper::Utf8ConvertToString(moduleRequest.GetTaggedValue());
    if (moduleManager->IsLocalModuleLoaded(moduleRequestStr)) {
        return JSHandle<JSTaggedValue>(moduleManager->HostGetImportedModule(moduleRequestStr));
    }

    CString dirname = base::PathHelper::ResolveDirPath(module->GetEcmaModuleFilenameString());
    CString moduleFilename = ResolveFilenameFromNative(thread, dirname, moduleRequestStr);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return ResolveImportedModuleBundlePack(thread, moduleFilename, executeFromJob);
}
CString ModuleResolver::ReplaceModuleThroughFeature(JSThread *thread, const CString &requestName)
{
    auto vm = thread->GetEcmaVM();
    // check if module need to be mock
    if (vm->IsMockModule(requestName)) {
        return vm->GetMockModule(requestName);
    }

    // Load the replaced module, hms -> system hsp
    if (vm->IsHmsModule(requestName)) {
        return vm->GetHmsModule(requestName);
    }
    return requestName;
}
JSHandle<JSTaggedValue> ModuleResolver::ResolveImportedModuleBundlePack(JSThread *thread,
                                                                        const CString &fileName,
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
    return HostResolveImportedModuleBundlePack(thread, fileName, executeFromJob);
}

JSHandle<JSTaggedValue> ModuleResolver::ResolveImportedModuleWithMerge(JSThread *thread,
                                                                       const CString &fileName,
                                                                       const CString &recordName,
                                                                       bool executeFromJob)
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
        CString msg = "cannot find record '" + recordName + "', please check the request path.'" + fileName + "'.";
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }

    if (jsPandaFile->IsSharedModule(recordInfo)) {
        return ResolveSharedImportedModuleWithMerge(thread, fileName, recordName, jsPandaFile.get(), recordInfo);
    }
    return HostResolveImportedModuleWithMerge(thread, fileName, recordName, executeFromJob);
}

JSHandle<JSTaggedValue> ModuleResolver::ResolveSharedImportedModuleWithMerge(JSThread *thread,
                                                                             const CString &fileName,
                                                                             const CString &recordName,
                                                                             const JSPandaFile *jsPandaFile,
                                                                             [[maybe_unused]] JSRecordInfo *recordInfo)
{
    auto sharedModuleManager = SharedModuleManager::GetInstance();
    if (sharedModuleManager->SearchInSModuleManager(thread, recordName)) {
        return JSHandle<JSTaggedValue>(sharedModuleManager->GetSModule(thread, recordName));
    }
    // before resolving module completely, shared-module put into isolate -thread resolvedModules_ temporarily.
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> module = moduleManager->TryGetImportedModule(recordName);
    if (!module->IsUndefined()) {
        return module;
    }

    ASSERT(jsPandaFile->IsModule(recordInfo));
    JSHandle<JSTaggedValue> moduleRecord =
        SharedModuleHelper::ParseSharedModule(thread, jsPandaFile, recordName, fileName, recordInfo);
    JSHandle<SourceTextModule>::Cast(moduleRecord)->SetEcmaModuleRecordNameString(recordName);
    moduleManager->AddResolveImportedModule(recordName, moduleRecord.GetTaggedValue());
    moduleManager->AddToInstantiatingSModuleList(recordName);
    return moduleRecord;
}
JSHandle<JSTaggedValue> ModuleResolver::HostResolveImportedModuleWithMerge(JSThread *thread,
                                                                           const CString &moduleFileName,
                                                                           const CString &recordName,
                                                                           bool executeFromJob)
{
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> module = moduleManager->TryGetImportedModule(recordName);
    if (!module->IsUndefined()) {
        return module;
    }
    return CommonResolveImportedModuleWithMerge(thread, moduleFileName, recordName, executeFromJob);
}

JSHandle<JSTaggedValue> ModuleResolver::HostResolveImportedModuleWithMergeForHotReload(JSThread *thread,
                                                                                       const CString &moduleFileName,
                                                                                       const CString &recordName,
                                                                                       bool executeFromJob)
{
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, recordName, false);
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << moduleFileName;
    }
    JSHandle<JSTaggedValue> moduleRecord =
        ResolveModuleWithMerge(thread, jsPandaFile.get(), recordName, executeFromJob);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->UpdateResolveImportedModule(recordName, moduleRecord.GetTaggedValue());
    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleResolver::CommonResolveImportedModuleWithMerge(JSThread *thread,
                                                                             const CString &moduleFileName,
                                                                             const CString &recordName,
                                                                             bool executeFromJob)
{
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, recordName, false);
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << moduleFileName;
    }
    JSHandle<JSTaggedValue> moduleRecord =
        ResolveModuleWithMerge(thread, jsPandaFile.get(), recordName, executeFromJob);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, moduleRecord.GetTaggedValue());
    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleResolver::HostResolveImportedModuleBundlePack(JSThread *thread,
                                                                            const CString &referencingModule,
                                                                            bool executeFromJob)
{
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CString moduleFileName = referencingModule;
    if (moduleManager->IsVMBundlePack()) {
        if (!AOTFileManager::GetAbsolutePath(referencingModule, moduleFileName)) {
            CString msg = "Parse absolute " + referencingModule + " path failed";
            THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
        }
    }

    JSHandle<JSTaggedValue> module = moduleManager->TryGetImportedModule(referencingModule);
    if (!module->IsUndefined()) {
        return module;
    }

    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << moduleFileName;
    }

    return ResolveModuleBundlePack(thread, jsPandaFile.get(), executeFromJob);
}

// The security interface needs to be modified accordingly.
JSHandle<JSTaggedValue> ModuleResolver::HostResolveImportedModuleBundlePack(JSThread *thread,
                                                                            const void *buffer,
                                                                            size_t size,
                                                                            const CString &filename)
{
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> module = moduleManager->TryGetImportedModule(filename);
    if (!module->IsUndefined()) {
        return module;
    }

    std::shared_ptr<JSPandaFile> jsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, filename,
        JSPandaFile::ENTRY_MAIN_FUNCTION, buffer, size);
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << filename;
    }

    return ResolveModuleBundlePack(thread, jsPandaFile.get());
}

JSHandle<JSTaggedValue> ModuleResolver::ResolveModuleBundlePack(JSThread *thread,
                                                                const JSPandaFile *jsPandaFile,
                                                                bool executeFromJob)
{
    CString moduleFileName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<JSTaggedValue> moduleRecord = thread->GlobalConstants()->GetHandledUndefined();
    JSRecordInfo recordInfo = const_cast<JSPandaFile *>(jsPandaFile)->FindRecordInfo(JSPandaFile::ENTRY_FUNCTION_NAME);
    if (jsPandaFile->IsModule(&recordInfo)) {
        moduleRecord =
            ModuleDataExtractor::ParseModule(thread, jsPandaFile, moduleFileName, moduleFileName, &recordInfo);
    } else {
        ASSERT(jsPandaFile->IsCjs(&recordInfo));
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, jsPandaFile);
    }
    // json file can not be compiled into isolate abc.
    ASSERT(!jsPandaFile->IsJson(&recordInfo));
    ModuleDeregister::InitForDeregisterModule(moduleRecord, executeFromJob);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->AddResolveImportedModule(moduleFileName, moduleRecord.GetTaggedValue());
    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleResolver::ResolveNativeModule(JSThread *thread,
                                                            const CString &moduleRequest,
                                                            const CString &baseFileName,
                                                            ModuleTypes moduleType)
{
    JSHandle<JSTaggedValue> moduleRecord =
        ModuleDataExtractor::ParseNativeModule(thread, moduleRequest, baseFileName, moduleType);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->AddResolveImportedModule(moduleRequest, moduleRecord.GetTaggedValue());
    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleResolver::ResolveModuleWithMerge(JSThread *thread,
                                                               const JSPandaFile *jsPandaFile,
                                                               const CString &recordName,
                                                               bool executeFromJob)
{
    CString moduleFileName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<JSTaggedValue> moduleRecord = thread->GlobalConstants()->GetHandledUndefined();
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(recordName, &recordInfo);
    if (!hasRecord) {
        JSHandle<JSTaggedValue> exp(thread, JSTaggedValue::Exception());
        THROW_MODULE_NOT_FOUND_ERROR_WITH_RETURN_VALUE(thread, recordName, moduleFileName, exp);
    }
    if (jsPandaFile->IsModule(recordInfo)) {
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, recordName, moduleFileName, recordInfo);
    } else if (jsPandaFile->IsJson(recordInfo)) {
        moduleRecord = ModuleDataExtractor::ParseJsonModule(thread, jsPandaFile, moduleFileName, recordName);
    } else {
        ASSERT(jsPandaFile->IsCjs(recordInfo));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, jsPandaFile);
    }

    JSHandle<SourceTextModule>::Cast(moduleRecord)->SetEcmaModuleRecordNameString(recordName);
    ModuleDeregister::InitForDeregisterModule(moduleRecord, executeFromJob);
    return moduleRecord;
}
JSHandle<JSTaggedValue> ModuleResolver::HostResolveImportedModuleBundlePack(JSThread *thread,
                                                                            const JSPandaFile *jsPandaFile,
                                                                            const CString &filename)
{
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << filename;
    }
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> module = moduleManager->TryGetImportedModule(filename);
    if (!module->IsUndefined()) {
        return module;
    }
    return ResolveModuleBundlePack(thread, jsPandaFile);
}
} // namespace panda::ecmascript