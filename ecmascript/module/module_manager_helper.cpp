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
#include "ecmascript/module/module_manager_helper.h"

#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
JSTaggedValue ModuleManagerHelper::GetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module, int index)
{
    ModuleTypes moduleType = module->GetTypes();
    if (SourceTextModule::IsNativeModule(moduleType) || SourceTextModule::IsCjsModule(moduleType)) {
        return GetNativeOrCjsModuleValue(thread, module.GetTaggedValue(), index);
    }
    return module->GetModuleValue(thread, index, false);
}

JSTaggedValue ModuleManagerHelper::GetNativeOrCjsModuleValue(JSThread *thread,
                                                             JSTaggedValue resolvedModule,
                                                             int32_t index)
{
    JSHandle<JSTaggedValue> exports = GetNativeOrCjsExports(thread, resolvedModule);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    return SourceTextModule::GetValueFromExportObject(thread, exports, index);
}

JSHandle<JSTaggedValue> ModuleManagerHelper::GetNativeOrCjsExports(JSThread *thread, JSTaggedValue resolvedModule)
{
    JSHandle<SourceTextModule> module(thread, resolvedModule);
    // if cjsModule is not JSObject, means cjs uses default exports.
    JSMutableHandle<JSTaggedValue> exports(thread, thread->GlobalConstants()->GetUndefined());
    ModuleTypes moduleType = module->GetTypes();
    if (SourceTextModule::IsNativeModule(moduleType)) {
        exports.Update(module->GetModuleValue(thread, 0, false));
        if (!exports->IsJSObject()) {
            CString errorMsg =
                "Loading native module:" + ConvertToString(SourceTextModule::GetModuleName(resolvedModule)) +
                ", failed";
            JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
            THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread,
                ErrorType::SYNTAX_ERROR, errorMsg.c_str(), exception);
        }
    }
    if (SourceTextModule::IsCjsModule(moduleType)) {
        JSHandle<JSTaggedValue> cjsModuleName(thread, SourceTextModule::GetModuleName(module.GetTaggedValue()));
        exports.Update(CjsModule::SearchFromModuleCache(thread, cjsModuleName).GetTaggedValue());
        if (exports->IsHole()) {
            CString errorMsg =
                "Loading cjs module:" + ConvertToString(SourceTextModule::GetModuleName(resolvedModule)) + ", failed";
            JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
            THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread,
                ErrorType::SYNTAX_ERROR, errorMsg.c_str(), exception);
        }
    }
    return exports;
}

JSTaggedValue ModuleManagerHelper::GetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module,
    JSTaggedValue bindingName)
{
    JSHandle<JSTaggedValue> exports = GetNativeOrCjsExports(thread, module.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    if (UNLIKELY(JSTaggedValue::SameValue(
        bindingName, thread->GlobalConstants()->GetHandledDefaultString().GetTaggedValue()))) {
        return exports.GetTaggedValue();
    }
    // need fix
    return JSHandle<JSTaggedValue>(thread, SlowRuntimeStub::LdObjByName(thread,
                                                                        exports.GetTaggedValue(),
                                                                        bindingName,
                                                                        false,
                                                                        JSTaggedValue::Undefined())).GetTaggedValue();
}

JSTaggedValue ModuleManagerHelper::GetModuleValueFromIndexBinding(JSThread *thread, JSHandle<SourceTextModule> module,
                                                                  JSTaggedValue resolvedBinding)
{
    JSHandle<ResolvedRecordIndexBinding> binding(thread, resolvedBinding);
    JSHandle<JSTaggedValue> recordName(thread, binding->GetModuleRecord());
    ASSERT(recordName->IsString());
    // recordName is string, find at current vm
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CString recordNameStr = ModulePathHelper::Utf8ConvertToString(recordName.GetTaggedValue());
    JSHandle<SourceTextModule> resolvedModule;
    if (moduleManager->IsEvaluatedModule(recordNameStr)) {
        resolvedModule = moduleManager->HostGetImportedModule(recordNameStr);
    } else {
        auto isMergedAbc = !module->GetEcmaModuleRecordName().IsUndefined();
        CString fileName = ModulePathHelper::Utf8ConvertToString((binding->GetAbcFileName()));
        if (!JSPandaFileExecutor::LazyExecuteModule(thread, recordNameStr, fileName, isMergedAbc)) {
            LOG_ECMA(FATAL) << "LazyExecuteModule failed";
        }
        resolvedModule = moduleManager->HostGetImportedModule(recordNameStr);
    }
    return GetModuleValue(thread, resolvedModule, binding->GetIndex());
}

JSTaggedValue ModuleManagerHelper::GetModuleValueFromRecordBinding(JSThread *thread, JSHandle<SourceTextModule> module,
                                                                   JSTaggedValue resolvedBinding)
{
    JSHandle<ResolvedRecordBinding> binding(thread, resolvedBinding);
    JSHandle<JSTaggedValue> recordName(thread, binding->GetModuleRecord());
    ASSERT(recordName->IsString());
    CString recordNameStr = ModulePathHelper::Utf8ConvertToString(recordName.GetTaggedValue());
    // moduleRecord is string, find at current vm
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> resolvedModule;
    if (moduleManager->IsEvaluatedModule(recordNameStr)) {
        resolvedModule = moduleManager->HostGetImportedModule(recordNameStr);
    } else {
        auto isMergedAbc = !module->GetEcmaModuleRecordName().IsUndefined();
        CString fileName = ModulePathHelper::Utf8ConvertToString((module->GetEcmaModuleFilename()));
        if (!JSPandaFileExecutor::LazyExecuteModule(thread, recordNameStr, fileName, isMergedAbc)) {
            LOG_ECMA(FATAL) << "LazyExecuteModule failed";
        }
        resolvedModule = moduleManager->HostGetImportedModule(recordNameStr);
    }
    return GetModuleValue(thread, resolvedModule, binding->GetBindingName());
}

JSTaggedValue ModuleManagerHelper::GetLazyModuleValueFromIndexBinding(JSThread *thread,
                                                                      JSHandle<SourceTextModule> module,
                                                                      JSTaggedValue resolvedBinding)
{
    JSHandle<ResolvedRecordIndexBinding> binding(thread, resolvedBinding);
    JSHandle<JSTaggedValue> recordName(thread, binding->GetModuleRecord());
    ASSERT(recordName->IsString());
    // moduleRecord is string, find at current vm
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CString recordNameStr = ModulePathHelper::Utf8ConvertToString(recordName.GetTaggedValue());
    JSHandle<SourceTextModule> resolvedModule;
    if (moduleManager->IsLocalModuleLoaded(recordNameStr)) {
        resolvedModule = moduleManager->HostGetImportedModule(recordNameStr);
        if (!moduleManager->IsEvaluatedModule(recordNameStr)) {
            SourceTextModule::Evaluate(thread, resolvedModule, nullptr);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        }
    } else {
        auto isMergedAbc = !module->GetEcmaModuleRecordName().IsUndefined();
        CString fileName = ModulePathHelper::Utf8ConvertToString(binding->GetAbcFileName());
        if (!JSPandaFileExecutor::LazyExecuteModule(thread, recordNameStr, fileName, isMergedAbc)) {
            LOG_ECMA(FATAL) << "LazyExecuteModule failed";
        }
        resolvedModule = moduleManager->HostGetImportedModule(recordNameStr);
    }
    return GetModuleValue(thread, resolvedModule, binding->GetIndex());
}

JSTaggedValue ModuleManagerHelper::GetLazyModuleValueFromRecordBinding(
    JSThread *thread, JSHandle<SourceTextModule> module, JSTaggedValue resolvedBinding)
{
    JSHandle<ResolvedRecordBinding> binding(thread, resolvedBinding);
    JSHandle<JSTaggedValue> recordName(thread, binding->GetModuleRecord());
    ASSERT(recordName->IsString());
    // moduleRecord is string, find at current vm
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CString recordNameStr = ModulePathHelper::Utf8ConvertToString(recordName.GetTaggedValue());
    JSHandle<SourceTextModule> resolvedModule;
    if (moduleManager->IsLocalModuleLoaded(recordNameStr)) {
        resolvedModule = moduleManager->HostGetImportedModule(recordNameStr);
        if (!moduleManager->IsEvaluatedModule(recordNameStr)) {
            SourceTextModule::Evaluate(thread, resolvedModule, nullptr);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        }
    } else {
        auto isMergedAbc = !module->GetEcmaModuleRecordName().IsUndefined();
        CString fileName = ModulePathHelper::Utf8ConvertToString(module->GetEcmaModuleFilename());
        if (!JSPandaFileExecutor::LazyExecuteModule(thread, recordNameStr, fileName, isMergedAbc)) {
            LOG_ECMA(FATAL) << "LazyExecuteModule failed";
        }
        resolvedModule = moduleManager->HostGetImportedModule(recordNameStr);
    }
    return GetModuleValue(thread, resolvedModule, binding->GetBindingName());
}

JSTaggedValue ModuleManagerHelper::UpdateBindingAndGetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module,
    JSHandle<SourceTextModule> requiredModule, int32_t index, JSTaggedValue bindingName)
{
    // Get esm environment
    JSHandle<JSTaggedValue> moduleEnvironment(thread, module->GetEnvironment());
    ASSERT(!moduleEnvironment->IsUndefined());
    JSHandle<TaggedArray> environment = JSHandle<TaggedArray>::Cast(moduleEnvironment);
    // rebinding here
    JSHandle<JSTaggedValue> exports = GetNativeOrCjsExports(thread, requiredModule.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    JSHandle<JSTaggedValue> exportName(thread, bindingName);
    JSHandle<JSTaggedValue> resolution =
        SourceTextModule::ResolveExportObject(thread, requiredModule, exports, exportName);
    // ii. If resolution is null or "ambiguous", throw a SyntaxError exception.
    if (resolution->IsNull() || resolution->IsString()) {
        CString requestMod = ModulePathHelper::ReformatPath(
            ModulePathHelper::Utf8ConvertToString(SourceTextModule::GetModuleName(requiredModule.GetTaggedValue())));
        CString recordStr = ModulePathHelper::ReformatPath(
            ModulePathHelper::Utf8ConvertToString(SourceTextModule::GetModuleName(module.GetTaggedValue())));
        CString msg = "the requested module '" + requestMod + SourceTextModule::GetResolveErrorReason(resolution) +
            ModulePathHelper::Utf8ConvertToString(bindingName) +
            "' which imported by '" + recordStr + "'";
        THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(
            thread, ErrorType::SYNTAX_ERROR, msg.c_str(), JSTaggedValue::Exception());
    }
    // iii. Call envRec.CreateImportBinding(
    // in.[[LocalName]], resolution.[[Module]], resolution.[[BindingName]]).
    environment->Set(thread, index, resolution);
    ASSERT(resolution->IsResolvedIndexBinding());
    ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolution.GetTaggedValue());
    return SourceTextModule::GetValueFromExportObject(thread, exports, binding->GetIndex());
}
} // namespace panda::ecmascript
