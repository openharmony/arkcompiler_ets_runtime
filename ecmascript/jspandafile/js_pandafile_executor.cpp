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

#include "ecmascript/js_file_path.h"
#include "ecmascript/jspandafile/abc_buffer_cache.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/checkpoint/thread_state_transition.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;

// use "@bundle" as ohmurl's rules, will be abandon later
std::pair<CString, CString> JSPandaFileExecutor::ParseAbcEntryPoint(JSThread *thread, const CString &filename,
    [[maybe_unused]] std::string_view entryPoint)
{
    CString name;
    CString entry;
    [[maybe_unused]] EcmaVM *vm = thread->GetEcmaVM();
#if defined(PANDA_TARGET_LINUX) || defined(OHOS_UNIT_TEST) || defined(PANDA_TARGET_MACOS)
    return {filename, entryPoint.data()};
#else
    CString normalName = PathHelper::NormalizePath(filename);
    ModulePathHelper::ParseAbcPathAndOhmUrl(vm, normalName, name, entry);
#if !defined(PANDA_TARGET_WINDOWS)
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
    return std::make_pair(name, entry);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromFile(JSThread *thread, const CString &name,
    CString entry, bool needUpdate, bool executeFromJob)
{
    EcmaVM *vm = thread->GetEcmaVM();

    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, name, entry, needUpdate);
    if (jsPandaFile == nullptr) {
#ifdef FUZZ_TEST
        CString msg = "jsPandaFile is nullptr";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
#else
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << name;
#endif
    }
    // If it is an old record, delete the bundleName and moduleName
    if (!jsPandaFile->IsBundlePack() && !vm->IsNormalizedOhmUrlPack() && !executeFromJob &&
        !vm->GetBundleName().empty()) {
        jsPandaFile->CheckIsRecordWithBundleName(entry);
        if (!jsPandaFile->IsRecordWithBundleName()) {
            PathHelper::AdaptOldIsaRecord(entry);
        }
    }

    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entry, &recordInfo);
    if (!hasRecord) {
        CString msg = "Cannot find module '" + entry + "' , which is application Entry Point";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
    }
    if (jsPandaFile->IsModule(recordInfo)) {
        ThreadManagedScope managedScope(thread);
        SharedModuleManager* sharedModuleManager = SharedModuleManager::GetInstance();
        JSHandle<JSTaggedValue> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
        if (jsPandaFile->IsBundlePack()) {
            moduleRecord = sharedModuleManager->ResolveImportedModule(thread, name, executeFromJob);
        } else {
            moduleRecord = sharedModuleManager->ResolveImportedModuleWithMerge(thread, name, entry, executeFromJob);
        }

        SourceTextModule::Instantiate(thread, moduleRecord, executeFromJob);
        if (thread->HasPendingException()) {
            return Unexpected(false);
        }
        JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
        module->SetStatus(ModuleStatus::INSTANTIATED);
        SourceTextModule::Evaluate(thread, module, nullptr, 0, executeFromJob);
        if (thread->HasPendingException()) {
            return Unexpected(false);
        }
        return JSTaggedValue::Undefined();
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile.get(), entry.c_str(), executeFromJob);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromAbsolutePathAbcFile(JSThread *thread,
    const CString &filename, std::string_view entryPoint, bool needUpdate, bool executeFromJob)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteFromAbsolutePathAbcFile filename " << filename;
    CString traceInfo = "JSPandaFileExecutor::ExecuteFromAbsolutePathAbcFile " + filename;
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    CString entry = entryPoint.data();
    CString name = filename;

    return ExecuteFromFile(thread, name, entry, needUpdate, executeFromJob);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromAbcFile(JSThread *thread, const CString &filename,
    std::string_view entryPoint, bool needUpdate, bool executeFromJob)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteFromAbcFile filename " << filename;
    CString traceInfo = "JSPandaFileExecutor::ExecuteFromAbcFile " + filename;
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    CString entry;
    CString name;
    EcmaVM *vm = thread->GetEcmaVM();
    if (!vm->IsBundlePack() && !executeFromJob) {
        std::tie(name, entry) = ParseAbcEntryPoint(thread, filename, entryPoint);
    } else {
        name = filename;
        entry = entryPoint.data();
    }

    return ExecuteFromFile(thread, name, entry, needUpdate, executeFromJob);
}

// The security interface needs to be modified accordingly.
Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromBuffer(JSThread *thread,
    const void *buffer, size_t size, std::string_view entryPoint, const CString &filename, bool needUpdate,
    bool executeFromJob)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteFromBuffer filename " << filename;
    CString traceInfo = "JSPandaFileExecutor::ExecuteFromBuffer " + filename;
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    CString normalName = PathHelper::NormalizePath(filename);
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, normalName, entryPoint, buffer, size, needUpdate);
    if (jsPandaFile == nullptr) {
#ifdef FUZZ_TEST
        CString msg = "jsPandaFile is nullptr";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
#else
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << normalName;
#endif
    }
    AbcBufferCacheScope bufferScope(thread, normalName, buffer, size, AbcBufferType::NORMAL_BUFFER);
    auto vm = thread->GetEcmaVM();

    CString entry = entryPoint.data();
    if (vm->IsNormalizedOhmUrlPack()) {
        entry = ModulePathHelper::TransformToNormalizedOhmUrl(vm, filename, normalName, entry);
    }
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entry, &recordInfo);
    if (!hasRecord) {
        CString msg = "Cannot find module '" + entry + "' , which is application Entry Point";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
    }
    if (jsPandaFile->IsModule(recordInfo)) {
        bool isBundle = jsPandaFile->IsBundlePack();
        return CommonExecuteBuffer(thread, isBundle, normalName, entry, buffer, size, executeFromJob);
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile.get(), entry, executeFromJob);
}

// The security interface needs to be modified accordingly.
Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteModuleBuffer(
    JSThread *thread, const void *buffer, size_t size, const CString &filename, bool needUpdate)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteModuleBuffer filename " << filename;
    CString traceInfo = "JSPandaFileExecutor::ExecuteModuleBuffer " + filename;
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    CString name;
    CString entry;
    EcmaVM *vm = thread->GetEcmaVM();
#if !defined(PANDA_TARGET_WINDOWS)
    name = vm->GetAssetPath();
#elif defined(PANDA_TARGET_WINDOWS)
    CString assetPath = vm->GetAssetPath();
    name = assetPath + "\\" + JSPandaFile::MERGE_ABC_NAME;
#else
    CString assetPath = vm->GetAssetPath();
    name = assetPath + "/" + JSPandaFile::MERGE_ABC_NAME;
#endif
    CString normalName = PathHelper::NormalizePath(filename);
    ModulePathHelper::ParseAbcPathAndOhmUrl(vm, normalName, name, entry);
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, name, entry, buffer, size, needUpdate);
    if (jsPandaFile == nullptr) {
#ifdef FUZZ_TEST
        CString msg = "jsPandaFile is nullptr";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
#else
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << name;
#endif
    }
    AbcBufferCacheScope bufferScope(thread, name, buffer, size, AbcBufferType::NORMAL_BUFFER);
    bool isBundle = jsPandaFile->IsBundlePack();

    // realEntry is used to record the original record, which is easy to throw when there are exceptions
    const CString realEntry = entry;
    if (vm->IsNormalizedOhmUrlPack()) {
        entry = ModulePathHelper::TransformToNormalizedOhmUrl(vm, filename, name, entry);
    } else if (!isBundle) {
        jsPandaFile->CheckIsRecordWithBundleName(entry);
        if (!jsPandaFile->IsRecordWithBundleName()) {
            PathHelper::AdaptOldIsaRecord(entry);
        }
    }
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entry, &recordInfo);
    if (!hasRecord) {
        CString msg = "Cannot find module '" + realEntry + "' , which is application Entry Point";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
    }
    if (!jsPandaFile->IsModule(recordInfo)) {
        LOG_ECMA(FATAL) << "Input file is not esmodule";
    }
    return CommonExecuteBuffer(thread, isBundle, name, entry, buffer, size);
}

// The security interface needs to be modified accordingly.
Expected<JSTaggedValue, bool> JSPandaFileExecutor::CommonExecuteBuffer(JSThread *thread,
    bool isBundle, const CString &filename, const CString &entry, const void *buffer, size_t size, bool executeFromJob)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->SetExecuteMode(ModuleExecuteMode::ExecuteBufferMode);
    JSMutableHandle<JSTaggedValue> moduleRecord(thread, thread->GlobalConstants()->GetUndefined());
    if (isBundle) {
        moduleRecord.Update(moduleManager->HostResolveImportedModule(buffer, size, filename));
    } else {
        moduleRecord.Update(moduleManager->HostResolveImportedModuleWithMerge(filename, entry, executeFromJob));
    }

    SourceTextModule::Instantiate(thread, moduleRecord, executeFromJob);
    if (thread->HasPendingException()) {
        return Unexpected(false);
    }

    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    module->SetStatus(ModuleStatus::INSTANTIATED);
    SourceTextModule::Evaluate(thread, module, buffer, size, executeFromJob);
    if (thread->HasPendingException()) {
        return Unexpected(false);
    }
    return JSTaggedValue::Undefined();
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::Execute(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                           std::string_view entryPoint, bool executeFromJob)
{
    ThreadManagedScope managedScope(thread);
    bool enableESMTrace = thread->GetEcmaVM()->GetJSOptions().EnableESMTrace();
    if (enableESMTrace) {
        CString traceInfo = "FileExecute: " + CString(entryPoint);
        ECMA_BYTRACE_START_TRACE(HITRACE_TAG_ARK, traceInfo.c_str());
    }
    // For Ark application startup
    EcmaContext *context = thread->GetCurrentEcmaContext();

    Expected<JSTaggedValue, bool> result;

    if (context->GetStageOfHotReload() == StageOfHotReload::BEGIN_EXECUTE_PATCHMAIN) {
        result = context->InvokeEcmaEntrypointForHotReload(jsPandaFile, entryPoint, executeFromJob);
    } else {
        QuickFixManager *quickFixManager = thread->GetEcmaVM()->GetQuickFixManager();
        quickFixManager->LoadPatchIfNeeded(thread, jsPandaFile);

        result = context->InvokeEcmaEntrypoint(jsPandaFile, entryPoint, executeFromJob);
    }
    if (enableESMTrace) {
        ECMA_BYTRACE_FINISH_TRACE(HITRACE_TAG_ARK);
    }
    return result;
}

void JSPandaFileExecutor::BindPreloadedPandaFilesToAOT(EcmaVM *vm, const std::string &moduleName)
{
    ASSERT(vm->GetJSThread()->IsMainThread());
    if (!vm->GetJSOptions().GetEnableAsmInterpreter()) {
        return;
    }
    // run not via command line
    if (vm->GetJSOptions().WasAOTOutputFileSet()) {
        return;
    }
    ASSERT(!moduleName.empty());
    // bind pandafiles loaded in appspawn
    vm->GetAOTFileManager()->BindPreloadedPandaFilesInAotFile(moduleName);
}

void JSPandaFileExecutor::BindPandaFileToAot(JSPandaFile *jsPandaFile)
{
    EcmaVM *vm = Runtime::GetInstance()->GetMainThread()->GetEcmaVM();
    if (vm->GetJSOptions().GetEnableAsmInterpreter()) {
        std::string aotFileBaseName(vm->GetModuleName());
        auto *aotFM = vm->GetAOTFileManager();
        if (vm->GetJSOptions().WasAOTOutputFileSet()) {
            std::string aotFilename = vm->GetJSOptions().GetAOTOutputFile();
            aotFileBaseName = JSFilePath::GetBaseName(aotFilename);
        }
        aotFM->BindPandaFileInAotFile(aotFileBaseName, jsPandaFile);
    }
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromBufferSecure(JSThread *thread, uint8_t *buffer,
    size_t size, std::string_view entryPoint, const CString &filename, bool needUpdate)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteFromBufferSecure with secure buffer filename " << filename;
    CString traceInfo = "JSPandaFileExecutor::ExecuteFromBufferSecure " + filename;
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    CString normalName = PathHelper::NormalizePath(filename);
    std::shared_ptr<JSPandaFile> jsPandaFile = JSPandaFileManager::GetInstance()->
        LoadJSPandaFileSecure(thread, normalName, entryPoint, buffer, size, needUpdate);
    if (jsPandaFile == nullptr) {
#ifdef FUZZ_TEST
        CString msg = "jsPandaFile is nullptr";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
#else
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << normalName;
#endif
    }
    AbcBufferCacheScope bufferScope(thread, normalName, buffer, size, AbcBufferType::SECURE_BUFFER);
    auto vm = thread->GetEcmaVM();

    CString entry = entryPoint.data();
    if (vm->IsNormalizedOhmUrlPack()) {
        entry = ModulePathHelper::TransformToNormalizedOhmUrl(vm, filename, normalName, entry);
    }
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entry, &recordInfo);
    if (!hasRecord) {
        CString msg = "Cannot find module '" + entry + "' , which is application Entry Point";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
    }
    if (jsPandaFile->IsModule(recordInfo)) {
        return CommonExecuteBuffer(thread, normalName, entry, jsPandaFile.get());
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile.get(), entry);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::CommonExecuteBuffer(JSThread *thread, const CString &filename,
    const CString &entry, const JSPandaFile *jsPandaFile)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->SetExecuteMode(ModuleExecuteMode::ExecuteBufferMode);
    JSMutableHandle<JSTaggedValue> moduleRecord(thread, thread->GlobalConstants()->GetUndefined());
    if (jsPandaFile->IsBundlePack()) {
        moduleRecord.Update(moduleManager->HostResolveImportedModule(jsPandaFile, filename));
    } else {
        moduleRecord.Update(moduleManager->HostResolveImportedModuleWithMerge(filename, entry));
    }

    SourceTextModule::Instantiate(thread, moduleRecord);
    if (thread->HasPendingException()) {
        return Unexpected(false);
    }

    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    module->SetStatus(ModuleStatus::INSTANTIATED);
    SourceTextModule::Evaluate(thread, module, nullptr, 0);
    if (thread->HasPendingException()) {
        return Unexpected(false);
    }
    return JSTaggedValue::Undefined();
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteModuleBufferSecure(JSThread *thread, uint8_t *buffer,
    size_t size, const CString &filename, bool needUpdate)
{
    LOG_ECMA(DEBUG) << "JSPandaFileExecutor::ExecuteModuleBufferSecure with secure buffer filename " << filename;
    CString traceInfo = "JSPandaFileExecutor::ExecuteModuleBufferSecure " + filename;
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    CString name;
    EcmaVM *vm = thread->GetEcmaVM();
#if !defined(PANDA_TARGET_WINDOWS)
    name = vm->GetAssetPath();
#elif defined(PANDA_TARGET_WINDOWS)
    CString assetPath = vm->GetAssetPath();
    name = assetPath + "\\" + JSPandaFile::MERGE_ABC_NAME;
#else
    CString assetPath = vm->GetAssetPath();
    name = assetPath + "/" + JSPandaFile::MERGE_ABC_NAME;
#endif
    CString entry;
    CString normalName = PathHelper::NormalizePath(filename);
    ModulePathHelper::ParseAbcPathAndOhmUrl(vm, normalName, name, entry);
    std::shared_ptr<JSPandaFile> jsPandaFile = JSPandaFileManager::GetInstance()->
        LoadJSPandaFileSecure(thread, name, entry, buffer, size, needUpdate);
    if (jsPandaFile == nullptr) {
#ifdef FUZZ_TEST
        CString msg = "jsPandaFile is nullptr";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
#else
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << name;
#endif
    }
    AbcBufferCacheScope bufferScope(thread, name, buffer, size, AbcBufferType::SECURE_BUFFER);
    // realEntry is used to record the original record, which is easy to throw when there are exceptions
    const CString realEntry = entry;
    if (vm->IsNormalizedOhmUrlPack()) {
        entry = ModulePathHelper::TransformToNormalizedOhmUrl(vm, filename, name, entry);
    } else if (!jsPandaFile->IsBundlePack()) {
        jsPandaFile->CheckIsRecordWithBundleName(entry);
        if (!jsPandaFile->IsRecordWithBundleName()) {
            PathHelper::AdaptOldIsaRecord(entry);
        }
    }

    // will be refactored, temporarily use the function IsModule to verify realEntry
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entry, &recordInfo);
    if (!hasRecord) {
        CString msg = "Cannot find module '" + realEntry + "' , which is application Entry Point";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
    }
    if (!jsPandaFile->IsModule(recordInfo)) {
        LOG_ECMA(FATAL) << "Input file is not esmodule";
    }
    return CommonExecuteBuffer(thread, name, entry, jsPandaFile.get());
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteSpecialModule(JSThread *thread, const CString &recordName,
    const CString &filename, const JSPandaFile *jsPandaFile, const JSRecordInfo* recordInfo)
{
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();

    if (jsPandaFile->IsCjs(recordInfo)) {
        moduleManager->ExecuteCjsModule(thread, recordName, jsPandaFile);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, Unexpected(false));
        return JSTaggedValue::Undefined();
    }
    if (jsPandaFile->IsJson(recordInfo)) {
        moduleManager->ExecuteJsonModule(thread, recordName, filename, jsPandaFile);
        return JSTaggedValue::Undefined();
    }
    UNREACHABLE();
    LOG_FULL(FATAL) << "this branch is unreachable";
}

// RecordName is the ohmurl-path of js files.
// The first js file executed could be json, cjs, native so or esm.
Expected<JSTaggedValue, bool> JSPandaFileExecutor::LazyExecuteModule(
    JSThread *thread, CString &recordName, const CString &filename, bool isMergedAbc)
{
    LOG_FULL(INFO) << "recordName : " << recordName << ", in abc : " << filename;
    CString traceInfo = "JSPandaFileExecutor::LazyExecuteModule " + filename;
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    CString newFileName = filename;
    if (newFileName.empty()) {
        newFileName = filename;
    }
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, newFileName, recordName);
    if (jsPandaFile == nullptr) {
#ifdef FUZZ_TEST
        CString msg = "jsPandaFile is nullptr";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
#else
        LOG_FULL(FATAL) << "Load file with filename '" << newFileName << "' failed, ";
#endif
    }

    // resolve native module
    auto [isNative, moduleType] = SourceTextModule::CheckNativeModule(recordName);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    if (isNative) {
        moduleManager->ExecuteNativeModule(thread, recordName);
        return JSTaggedValue::Undefined();
    }

    if (isMergedAbc && !jsPandaFile->HasRecord(recordName)) {
        CString msg = "cannot find record '" + recordName + "', in lazy load abc: " + newFileName;
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), Unexpected(false));
    }

    const JSRecordInfo* recordInfo = jsPandaFile->GetRecordInfo(recordName);
    if (!jsPandaFile->IsModule(recordInfo)) {
        return JSPandaFileExecutor::ExecuteSpecialModule(thread, recordName, newFileName, jsPandaFile.get(),
            recordInfo);
    }
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // The first js file should execute at current vm.
    JSHandle<JSTaggedValue> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
    if (isMergedAbc) {
        moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(newFileName, recordName);
    } else {
        moduleRecord = moduleManager->HostResolveImportedModule(newFileName);
    }
    SourceTextModule::Instantiate(thread, moduleRecord);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, Unexpected(false));
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    SourceTextModule::Evaluate(thread, module, nullptr, 0);
    return JSTaggedValue::Undefined();
}

int JSPandaFileExecutor::ExecuteAbcFileWithSingletonPatternFlag(JSThread *thread,
    [[maybe_unused]] const CString &bundleName, const CString &moduleName, const CString &entry,
    bool isSingletonPattern)
{
    CString abcFilePath = ModulePathHelper::ConcatPandaFilePath(moduleName);
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, abcFilePath, entry);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "When the route jump, loading panda file failed. Current file is " << abcFilePath;
        return ROUTE_URI_ERROR;
    }
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, jsPandaFile.get(),
        abcFilePath, "", entry);
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entryPoint, &recordInfo);
    if (!hasRecord) {
        LOG_ECMA(ERROR) << "When the route jump, Cannot find module '" << entryPoint << "'";
        return ROUTE_URI_ERROR;
    }
    ASSERT(jsPandaFile->IsModule(recordInfo));
    [[maybe_unused]] EcmaHandleScope scope(thread);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> moduleRecord(thread->GlobalConstants()->GetHandledUndefined());
    ASSERT(!jsPandaFile->IsBundlePack());
    moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(abcFilePath, entryPoint);
    SourceTextModule::Instantiate(thread, moduleRecord);
    if (thread->HasPendingException()) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ROUTE_INTERNAL_ERROR);
    }
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    if (!isSingletonPattern) {
        LOG_ECMA(INFO) << "Route jump to non-singleton page: " << entryPoint;
        module->SetStatus(ModuleStatus::INSTANTIATED);
    } else {
        LOG_ECMA(INFO) << "Route jump to singleton page: " << entryPoint;
    }
    SourceTextModule::Evaluate(thread, module, nullptr, 0);
    if (thread->HasPendingException()) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ROUTE_INTERNAL_ERROR);
    }
    return ROUTE_SUCCESS;
}

bool JSPandaFileExecutor::IsExecuteModuleInAbcFile(JSThread *thread, [[maybe_unused]] const CString &bundleName,
    const CString &moduleName, const CString &entry)
{
    CString abcFilePath = ModulePathHelper::ConcatPandaFilePath(moduleName);
    bool isValid = JSPandaFileManager::GetInstance()->CheckFilePath(thread, abcFilePath);
    if (!isValid) {
        return false;
    }
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, abcFilePath, entry);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "When checking if module is in abc file, loading panda file failed. Current file is " <<
            abcFilePath;
        return false;
    }
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, jsPandaFile.get(),
        abcFilePath, "", entry);
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entryPoint, &recordInfo);
    if (!hasRecord) {
        LOG_ECMA(ERROR) << "When checking if module is in abc file, Cannot find module '" << entryPoint << "'";
        return false;
    }
    return true;
}
}  // namespace panda::ecmascript
