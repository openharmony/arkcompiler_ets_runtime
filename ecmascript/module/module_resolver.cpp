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
        return moduleManager->ResolveNativeModule(requestStr, baseFilename, moduleType);
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
    return SharedModuleManager::GetInstance()->ResolveImportedModuleWithMerge(thread, outFileName, entryPoint,
        executeFromJob);
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
    return SharedModuleManager::GetInstance()->ResolveImportedModule(thread, moduleFilename, executeFromJob);
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
} // namespace panda::ecmascript