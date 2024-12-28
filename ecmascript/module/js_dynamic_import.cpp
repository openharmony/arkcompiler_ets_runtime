/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/module/js_dynamic_import.h"
#include "ecmascript/base/path_helper.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_resolver.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;
using BuiltinsPromiseJob = builtins::BuiltinsPromiseJob;

JSTaggedValue DynamicImport::ExecuteNativeOrJsonModule(JSThread *thread, const CString &specifierString,
                                                       ModuleTypes moduleType,
                                                       JSHandle<JSPromiseReactionsFunction> resolve,
                                                       JSHandle<JSPromiseReactionsFunction> reject,
                                                       const JSPandaFile *jsPandaFile)
{
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSMutableHandle<JSTaggedValue> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
    // IsInstantiatedModule is for lazy module to execute
    if (moduleManager->IsLocalModuleLoaded(specifierString) &&
        (!moduleManager->IsLocalModuleInstantiated(specifierString))) {
        ModuleDeregister::ReviseLoadedModuleCount(thread, specifierString);
        JSHandle<SourceTextModule> moduleRecord =
            moduleManager->HostGetImportedModule(specifierString);
        moduleRecord->CheckAndThrowModuleError(thread);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, BuiltinsPromiseJob::CatchException(thread, reject));
        requiredModule.Update(moduleRecord);
    } else {
        JSHandle<SourceTextModule> moduleRecord(thread, thread->GlobalConstants()->GetUndefined());
        if (moduleType != ModuleTypes::JSON_MODULE) {
            // nativeModule
            JSHandle<JSTaggedValue> nativeModuleHld =
                ModuleResolver::ResolveNativeModule(thread, specifierString, "", moduleType);
            moduleRecord = JSHandle<SourceTextModule>::Cast(nativeModuleHld);
            if (!SourceTextModule::EvaluateNativeModule(thread, moduleRecord, moduleType)) {
                LOG_FULL(ERROR) << " dynamically loading native module" << specifierString << " failed";
            }
        } else {
            // json
            moduleRecord = JSHandle<SourceTextModule>::Cast(ModuleDataExtractor::ParseJsonModule(
                thread, jsPandaFile, jsPandaFile->GetJSPandaFileDesc(), specifierString));
            SourceTextModule::RecordEvaluatedOrError(thread, moduleRecord);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, BuiltinsPromiseJob::CatchException(thread, reject));
        }
        moduleManager->AddResolveImportedModule(specifierString, moduleRecord.GetTaggedValue());
        moduleRecord->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
        moduleRecord->SetRegisterCounts(1);
        thread->GetEcmaVM()->PushToDeregisterModuleList(specifierString);
        requiredModule.Update(moduleRecord);
    }

    JSHandle<JSTaggedValue> moduleNamespace = SourceTextModule::GetModuleNamespace(thread,
        JSHandle<SourceTextModule>::Cast(requiredModule));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, BuiltinsPromiseJob::CatchException(thread, reject));
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread,
                                            JSHandle<JSTaggedValue>(resolve),
                                            undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, BuiltinsPromiseJob::CatchException(thread, reject));
    info->SetCallArg(moduleNamespace.GetTaggedValue());
    return JSFunction::Call(info);
}
}  // namespace panda::ecmascript
