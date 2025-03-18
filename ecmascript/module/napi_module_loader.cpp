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

#include "ecmascript/module/napi_module_loader.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> NapiModuleLoader::LoadModuleNameSpaceWithModuleInfo(EcmaVM *vm, CString &requestPath,
    CString &modulePath, CString &abcFilePath)
{
    LOG_ECMA(INFO) << "NapiModuleLoader::LoadModuleNameSpaceWithModuleInfo requestPath:" << requestPath <<
        "," << "modulePath:" << modulePath;
    JSThread *thread = vm->GetJSThread();
    std::shared_ptr<JSPandaFile> curJsPandaFile;
    if (!modulePath.empty()) {
        curJsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(
            thread, abcFilePath, requestPath, false, ExecuteTypes::NAPI);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        if (curJsPandaFile == nullptr) {
            LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << abcFilePath;
        }
        if (vm->IsNormalizedOhmUrlPack()) {
            ModulePathHelper::TranslateExpressionToNormalized(thread, curJsPandaFile.get(), abcFilePath, "",
                requestPath);
        } else if (ModulePathHelper::NeedTranstale(requestPath)) {
            ModulePathHelper::TranstaleExpressionInput(curJsPandaFile.get(), requestPath);
        }
    }
    JSHandle<JSTaggedValue> nameSp = LoadModuleNameSpaceWithPath(thread, abcFilePath, requestPath, modulePath,
        curJsPandaFile.get());
    return nameSp;
}

JSHandle<JSTaggedValue> NapiModuleLoader::LoadModuleNameSpace(EcmaVM *vm, CString requestPath,
    const CString& moduleName, CString& abcFilePath)
{
    JSThread *thread = vm->GetJSThread();
    CString path = base::ConcatToCString(vm->GetBundleName(), PathHelper::SLASH_TAG);
    // RequestPath starts with ets/xxx
    if (StringHelper::StringStartWith(requestPath, ModulePathHelper::PREFIX_ETS)) {
        path += moduleName;
        CString recordNameStr = ModulePathHelper::TranslateNapiFileRequestPath(thread, path, requestPath);
        LOG_ECMA(DEBUG) << "NapiModuleLoader::LoadFilePathWithinModule: Concated recordName " << recordNameStr;
        return LoadModuleNameSpaceFromFile(thread, recordNameStr, abcFilePath);
    }
    CString abcModuleName = ModulePathHelper::GetModuleNameWithBaseFile(abcFilePath);
    CString srcPrefix = base::ConcatToCString(abcModuleName, ModulePathHelper::PHYCICAL_FILE_PATH);
    path += abcModuleName;
    // RequestPath starts with moduleName/src/main/xxx
    if (StringHelper::StringStartWith(requestPath, srcPrefix)) {
        return LoadFilePathWithinModule(thread, abcFilePath, srcPrefix, requestPath, path);
    }
    return LoadModuleNameSpaceWithModuleInfo(vm, requestPath, path, abcFilePath);
}

JSHandle<JSTaggedValue> NapiModuleLoader::LoadModuleNameSpace(EcmaVM *vm, CString requestPath, CString modulePath)
{
    JSThread *thread = vm->GetJSThread();
    CString moduleName = ModulePathHelper::GetModuleNameWithPath(modulePath);
    CString abcFilePath = ModulePathHelper::ConcatPandaFilePath(moduleName);
    CString srcPrefix = base::ConcatToCString(moduleName, ModulePathHelper::PHYCICAL_FILE_PATH);
    // RequestPath starts with moduleName/src/main/xxx
    if (StringHelper::StringStartWith(requestPath, srcPrefix)) {
        return LoadFilePathWithinModule(thread, abcFilePath, srcPrefix, requestPath, modulePath);
    }
    return LoadModuleNameSpaceWithModuleInfo(vm, requestPath, modulePath, abcFilePath);
}

JSHandle<JSTaggedValue> NapiModuleLoader::LoadFilePathWithinModule(JSThread *thread, const CString& abcFilePath,
    const CString& srcPrefix, const CString& requestPath, const CString& modulePath)
{
    if (requestPath.size() > srcPrefix.size()) {
        // Sub after moduleName/src/main/
        CString fileName = requestPath.substr(srcPrefix.size() + 1);
        CString recordNameStr = ModulePathHelper::TranslateNapiFileRequestPath(thread, modulePath, fileName);
        LOG_ECMA(DEBUG) << "NapiModuleLoader::LoadFilePathWithinModule: Concated recordName " << recordNameStr;
        return LoadModuleNameSpaceFromFile(thread, recordNameStr, abcFilePath);
    } else {
        CString msg = "cannot find record '" + requestPath + "' in basefileName " + abcFilePath + "," +
            "from napi load module";
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }
}

JSHandle<JSTaggedValue> NapiModuleLoader::LoadModuleNameSpaceWithPath(JSThread *thread, CString &abcFilePath,
    const CString &requestPath, const CString &modulePath, const JSPandaFile *pandaFile)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    if (SourceTextModule::IsNativeModule(requestPath)) {
        JSHandle<JSTaggedValue> moduleHandle = moduleManager->LoadNativeModule(thread, requestPath);
        return moduleHandle;
    }
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, pandaFile,
        abcFilePath, modulePath, requestPath);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return LoadModuleNameSpaceFromFile(thread, entryPoint, abcFilePath);
}

JSHandle<JSTaggedValue> NapiModuleLoader::LoadModuleNameSpaceFromFile(
    JSThread *thread, const CString &entryPoint, const CString &abcFilePath)
{
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, abcFilePath, entryPoint, false, ExecuteTypes::NAPI);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    if (jsPandaFile == nullptr) {
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << abcFilePath;
    }
    JSRecordInfo *recordInfo = jsPandaFile->CheckAndGetRecordInfo(entryPoint);
    if (recordInfo == nullptr) {
        LOG_FULL(ERROR) << "cannot find record '" << entryPoint <<"' in basefileName " << abcFilePath << ","
            << "from napi load module";
        CString msg = "cannot find record '" + entryPoint + "' in basefileName " + abcFilePath + "," +
            "from napi load module";
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }

    if (jsPandaFile->IsSharedModule(recordInfo)) {
        LockHolder lock(SharedModuleManager::GetInstance()->GetSharedMutex());
        return ecmascript::NapiModuleLoader::GetModuleNameSpace(
            thread, entryPoint, abcFilePath);
    }
    return ecmascript::NapiModuleLoader::GetModuleNameSpace(
        thread, entryPoint, abcFilePath);
}

JSHandle<JSTaggedValue> NapiModuleLoader::GetModuleNameSpace(JSThread *thread, const CString &entryPoint,
    const CString &abcFilePath)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    // IsInstantiatedModule is for lazy module to execute
    if (moduleManager->NeedExecuteModule(entryPoint)) {
        if (!JSPandaFileExecutor::ExecuteFromAbcFile(
            thread, abcFilePath, entryPoint, false, ExecuteTypes::NAPI)) {
            CString msg = "Cannot execute request from napi load module : " + entryPoint +
                ", from napi load module";
            THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
        }
    }

    JSHandle<SourceTextModule> moduleRecord = moduleManager->GetImportedModule(entryPoint);
    auto &options = const_cast<EcmaVM *>(thread->GetEcmaVM())->GetJSOptions();
    if (options.EnableModuleException()) {
        moduleRecord->CheckAndThrowModuleError(thread);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    }
    JSHandle<JSTaggedValue> nameSp = SourceTextModule::GetModuleNamespace(thread, moduleRecord);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return nameSp;
}
}