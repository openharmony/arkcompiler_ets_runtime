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

#include "ecmascript/base/path_helper.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/require/js_cjs_module_cache.h"
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
    JSMutableHandle<JSTaggedValue> parent(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> dirname(thread, JSTaggedValue::Undefined());
    const JSPandaFile *jsPandaFile = EcmaInterpreter::GetNativeCallPandafile(thread);
    JSHandle<JSTaggedValue> entrypointVal(thread, EcmaInterpreter::GetCurrentEntryPoint(thread));
    CString mergedFilename = jsPandaFile->GetJSPandaFileDesc();
    CString requestEntryPoint = JSPandaFile::ENTRY_MAIN_FUNCTION;

    JSMutableHandle<JSTaggedValue> filename(thread, JSTaggedValue::Undefined());
    if (!jsPandaFile->IsMergedPF()) {
        ModulePathHelper::ResolveCurrentPath(thread, parent, dirname, jsPandaFile);
        filename.Update(ResolveFilenameFromNative(thread, dirname.GetTaggedValue(),
                                                  request.GetTaggedValue()));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        mergedFilename = ConvertToString(filename.GetTaggedValue());
    } else {
        CString currentEntryPoint = ConvertToString(entrypointVal.GetTaggedValue());
        CString requestStr = ConvertToString(request.GetTaggedValue());
        requestEntryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, jsPandaFile, mergedFilename,
                                                                      currentEntryPoint, requestStr);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        filename.Update(factory->NewFromUtf8(requestEntryPoint));
    }

    // Search from Module.cache
    JSHandle<JSTaggedValue> maybeCachedExports = SearchFromModuleCache(thread, filename);
    if (!maybeCachedExports->IsHole()) {
        return maybeCachedExports;
    }

    // Don't get required exports from cache, execute required JSPandaFile.
    // module = new Module(), which belongs to required JSPandaFile.
    JSHandle<CjsModule> module = factory->NewCjsModule();
    CString fullName = ConvertToString(filename.GetTaggedValue());
    dirname.Update(PathHelper::ResolveDirPath(thread, fullName));
    InitializeModule(thread, module, filename, dirname);
    PutIntoCache(thread, module, filename);

    JSRecordInfo recordInfo;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(requestEntryPoint, recordInfo);
    if (!hasRecord) {
        CString msg = "cannot find record '" + requestEntryPoint + "', please check the request path.";
        LOG_FULL(ERROR) << msg;
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }
    if (jsPandaFile->IsJson(recordInfo)) {
        JSHandle<JSTaggedValue> result = JSHandle<JSTaggedValue>(thread,
            ModuleDataExtractor::JsonParse(thread, jsPandaFile, requestEntryPoint));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        // Set module.exports ---> exports
        JSHandle<JSTaggedValue> exportsKey = thread->GlobalConstants()->GetHandledCjsExportsString();
        SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), exportsKey.GetTaggedValue(),
                                     result.GetTaggedValue());
        return result;
    }
    // Execute required JSPandaFile
    RequireExecution(thread, mergedFilename, requestEntryPoint);
    if (thread->HasPendingException()) {
        thread->GetCurrentEcmaContext()->HandleUncaughtException(thread->GetException());
        return thread->GlobalConstants()->GetHandledUndefined();
    }
    // Search from Module.cache after execution.
    JSHandle<JSTaggedValue> cachedExports = SearchFromModuleCache(thread, filename);
    if (cachedExports->IsHole()) {
        LOG_ECMA(FATAL) << "CJS REQUIRE FAIL : Can not obtain module, after executing required jsPandaFile";
        UNREACHABLE();
    }
    return cachedExports;
}

void CjsModule::RequireExecution(JSThread *thread, CString mergedFilename, CString requestEntryPoint)
{
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, mergedFilename, requestEntryPoint);
    if (jsPandaFile == nullptr) {
        CString msg = "Load file with filename '" + mergedFilename + "' failed, recordName '" + requestEntryPoint + "'";
        THROW_ERROR(thread, ErrorType::REFERENCE_ERROR, msg.c_str());
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