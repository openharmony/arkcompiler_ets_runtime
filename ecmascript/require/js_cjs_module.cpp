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

#include "ecmascript/require/js_cjs_module.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/require/js_require_manager.h"

namespace panda::ecmascript {

void CjsModule::InitializeModule(JSThread *thread, JSHandle<CjsModule> &module,
                                 JSHandle<JSTaggedValue> &filename, JSHandle<JSTaggedValue> &dirname)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> dirKey(factory->NewFromASCII("path"));
    SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), dirKey.GetTaggedValue(),
                                 dirname.GetTaggedValue());
    JSHandle<JSTaggedValue> filenameKey(factory->NewFromASCII("filename"));
    SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), filenameKey.GetTaggedValue(),
                                 filename.GetTaggedValue());
    module->SetFilename(thread, filename.GetTaggedValue());
    module->SetPath(thread, dirname.GetTaggedValue());
    return;
}

JSHandle<JSTaggedValue> CjsModule::SearchFromModuleCache(JSThread *thread, JSHandle<JSTaggedValue> &filename)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> moduleObj(env->GetCjsModuleFunction());
    JSHandle<JSTaggedValue> cacheName = globalConst->GetHandledCjsCacheString();
    JSTaggedValue modCache = SlowRuntimeStub::LdObjByName(thread, moduleObj.GetTaggedValue(),
                                                          cacheName.GetTaggedValue(),
                                                          false,
                                                          JSTaggedValue::Undefined());
    JSHandle<CjsModuleCache> moduleCache = JSHandle<CjsModuleCache>(thread, modCache);
    if (moduleCache->ContainsModule(filename.GetTaggedValue())) {
        JSHandle<CjsModule> cachedModule = JSHandle<CjsModule>(thread,
                                                               moduleCache->GetModule(filename.GetTaggedValue()));
        JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
        JSTaggedValue cachedExports = SlowRuntimeStub::LdObjByName(thread, cachedModule.GetTaggedValue(),
                                                                   exportsName.GetTaggedValue(),
                                                                   false,
                                                                   JSTaggedValue::Undefined());

        return JSHandle<JSTaggedValue>(thread, cachedExports);
    }
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Hole());
}

void CjsModule::PutIntoCache(JSThread *thread, JSHandle<CjsModule> &module, JSHandle<JSTaggedValue> &filename)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> moduleObj(env->GetCjsModuleFunction());
    JSHandle<JSTaggedValue> cacheName = globalConst->GetHandledCjsCacheString();
    JSTaggedValue modCache = SlowRuntimeStub::LdObjByName(thread, moduleObj.GetTaggedValue(),
                                                          cacheName.GetTaggedValue(),
                                                          false,
                                                          JSTaggedValue::Undefined());
    JSHandle<CjsModuleCache> moduleCache = JSHandle<CjsModuleCache>(thread, modCache);
    JSHandle<JSTaggedValue> moduleHandle = JSHandle<JSTaggedValue>::Cast(module);
    JSHandle<CjsModuleCache> newCache = CjsModuleCache::PutIfAbsentAndReset(thread, moduleCache, filename,
        moduleHandle);
    SlowRuntimeStub::StObjByName(thread, moduleObj.GetTaggedValue(), cacheName.GetTaggedValue(),
                                 newCache.GetTaggedValue());
}

JSHandle<JSTaggedValue> CjsModule::Load(JSThread *thread, JSHandle<EcmaString> &request)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Get local jsPandaFile's dirPath
    const JSPandaFile *jsPandaFile = EcmaInterpreter::GetNativeCallPandafile(thread);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(FATAL) << "CJS REQUIRE FAIL : jsPandaFile is null";
        UNREACHABLE();
    }
    CString filename = jsPandaFile->GetJSPandaFileDesc();
    CString requestEntryPoint = JSPandaFile::ENTRY_MAIN_FUNCTION;
    CString requestStr = ModulePathHelper::Utf8ConvertToString(request.GetTaggedValue());
    CString parent;
    CString dirname;
    CString recordName;
    if (jsPandaFile->IsBundlePack()) {
        ModulePathHelper::ResolveCurrentPath(parent, dirname, jsPandaFile);
        recordName = ResolveFilenameFromNative(thread, dirname, requestStr);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        filename = recordName;
    } else {
        CString currentEntryPoint = EcmaInterpreter::GetCurrentEntryPoint(thread).first;
        requestEntryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, jsPandaFile, filename,
                                                                      currentEntryPoint, requestStr);
        recordName = requestEntryPoint;
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    }

    // Search from Module.cache
    JSHandle<JSTaggedValue> recordNameHdl = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(recordName));
    JSHandle<JSTaggedValue> maybeCachedExports = SearchFromModuleCache(thread, recordNameHdl);
    if (!maybeCachedExports->IsHole()) {
        return maybeCachedExports;
    }

    // Don't get required exports from cache, execute required JSPandaFile.
    // module = new Module(), which belongs to required JSPandaFile.
    JSHandle<CjsModule> module = factory->NewCjsModule();
    dirname = PathHelper::ResolveDirPath(recordName);
    JSHandle<JSTaggedValue> dirnameHdl = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(dirname));
    InitializeModule(thread, module, recordNameHdl, dirnameHdl);
    PutIntoCache(thread, module, recordNameHdl);

    JSRecordInfo *recordInfo = jsPandaFile->CheckAndGetRecordInfo(requestEntryPoint);
    if (recordInfo == nullptr) {
        JSHandle<JSTaggedValue> exp(thread, JSTaggedValue::Exception());
        THROW_MODULE_NOT_FOUND_ERROR_WITH_RETURN_VALUE(thread, requestStr, recordName, exp);
    }
    if (jsPandaFile->IsJson(recordInfo)) {
        JSHandle<JSTaggedValue> result = JSHandle<JSTaggedValue>(thread,
            ModuleDataExtractor::JsonParse(thread, jsPandaFile, requestEntryPoint));
        // Set module.exports ---> exports
        JSHandle<JSTaggedValue> exportsKey = thread->GlobalConstants()->GetHandledCjsExportsString();
        SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), exportsKey.GetTaggedValue(),
                                     result.GetTaggedValue());
        return result;
    }
    // Execute required JSPandaFile
    RequireExecution(thread, filename, requestEntryPoint);
    if (thread->HasPendingException()) {
        thread->GetCurrentEcmaContext()->HandleUncaughtException();
        return thread->GlobalConstants()->GetHandledUndefined();
    }
    // Search from Module.cache after execution.
    JSHandle<JSTaggedValue> cachedExports = SearchFromModuleCache(thread, recordNameHdl);
    if (cachedExports->IsHole()) {
        LOG_ECMA(FATAL) << "CJS REQUIRE FAIL : Can not obtain module, after executing required jsPandaFile";
        UNREACHABLE();
    }
    return cachedExports;
}

void CjsModule::RequireExecution(JSThread *thread, const CString &mergedFilename, const CString &requestEntryPoint)
{
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(
            thread, mergedFilename, requestEntryPoint, false, ExecuteTypes::STATIC);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    if (jsPandaFile == nullptr) {
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " <<  mergedFilename;
    }
    JSPandaFileExecutor::Execute(thread, jsPandaFile.get(), requestEntryPoint);
}

JSTaggedValue CjsModule::Require(JSThread *thread, JSHandle<EcmaString> &request,
                                 [[maybe_unused]] JSHandle<CjsModule> &parent,
                                 [[maybe_unused]] bool isMain)
{
    Load(thread, request);
    return JSTaggedValue::Undefined();
}
} // namespace panda::ecmascript