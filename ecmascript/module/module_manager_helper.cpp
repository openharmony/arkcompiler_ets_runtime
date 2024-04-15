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
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
JSTaggedValue ModuleManagerHelper::GetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module, int index)
{
    ModuleTypes moduleType = module->GetTypes();
    if (SourceTextModule::IsNativeModule(moduleType)) {
        JSHandle<JSTaggedValue> nativeExports = JSHandle<JSTaggedValue>(thread,
            module->GetModuleValue(thread, 0, false));
        if (!nativeExports->IsJSObject()) {
            JSHandle<JSTaggedValue> recordName(thread, module->GetEcmaModuleRecordName());
            LOG_FULL(WARN) << "Load native module failed, so is " <<
                ConvertToString(recordName.GetTaggedValue());
            return nativeExports.GetTaggedValue();
        }
        return SourceTextModule::GetValueFromExportObject(thread, nativeExports, index);
    }
    if (SourceTextModule::IsCjsModule(moduleType)) {
        JSHandle<JSTaggedValue> cjsModuleName(thread, SourceTextModule::GetModuleName(module.GetTaggedValue()));
        JSHandle<JSTaggedValue> cjsExports = CjsModule::SearchFromModuleCache(thread, cjsModuleName);
        if (cjsExports->IsHole()) {
            LOG_FULL(FATAL) << "Load cjs module failed,  is " << ConvertToString(cjsModuleName.GetTaggedValue());
        }
        return SourceTextModule::GetValueFromExportObject(thread, cjsExports, index);
    }
    return module->GetModuleValue(thread, index, false);
}

JSTaggedValue ModuleManagerHelper::GetNativeModuleValue(JSThread *thread,
    JSTaggedValue resolvedModule, ResolvedBinding *binding)
{
    JSHandle<JSTaggedValue> nativeExports = JSHandle<JSTaggedValue>(thread,
        SourceTextModule::Cast(resolvedModule.GetTaggedObject())->GetModuleValue(thread, 0, false));
    if (!nativeExports->IsJSObject()) {
        JSHandle<JSTaggedValue> nativeModuleName(thread, SourceTextModule::GetModuleName(resolvedModule));
        LOG_FULL(WARN) << "Load native module failed, so is " <<
            ConvertToString(nativeModuleName.GetTaggedValue());
        return nativeExports.GetTaggedValue();
    }
    if (UNLIKELY(JSTaggedValue::SameValue(binding->GetBindingName(),
        thread->GlobalConstants()->GetHandledDefaultString().GetTaggedValue()))) {
        return nativeExports.GetTaggedValue();
    }
    return JSHandle<JSTaggedValue>(thread, SlowRuntimeStub::LdObjByName(thread,
                                                                        nativeExports.GetTaggedValue(),
                                                                        binding->GetBindingName(),
                                                                        false,
                                                                        JSTaggedValue::Undefined())).GetTaggedValue();
}

JSTaggedValue ModuleManagerHelper::GetNativeModuleValue(JSThread *thread, JSTaggedValue resolvedModule, int32_t index)
{
    DISALLOW_GARBAGE_COLLECTION;
    SourceTextModule *module = SourceTextModule::Cast(resolvedModule.GetTaggedObject());
    JSHandle<JSTaggedValue> nativeExports = JSHandle<JSTaggedValue>(thread,
        module->GetModuleValue(thread, 0, false));
    if (!nativeExports->IsJSObject()) {
        JSHandle<JSTaggedValue> recordName(thread, module->GetEcmaModuleRecordName());
        LOG_FULL(WARN) << "Load native module failed, so is " <<
            ConvertToString(recordName.GetTaggedValue());
        return nativeExports.GetTaggedValue();
    }
    return SourceTextModule::GetValueFromExportObject(thread, nativeExports, index);
}

JSTaggedValue ModuleManagerHelper::GetCJSModuleValue(JSThread *thread, JSTaggedValue resolvedModule, int32_t index)
{
    JSHandle<SourceTextModule> module(thread, resolvedModule);
    JSHandle<JSTaggedValue> cjsModuleName(thread, SourceTextModule::GetModuleName(resolvedModule));
    JSHandle<JSTaggedValue> cjsExports = CjsModule::SearchFromModuleCache(thread, cjsModuleName);
    // if cjsModule is not JSObject, means cjs uses default exports.
    if (!cjsExports->IsJSObject()) {
        if (cjsExports->IsHole()) {
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            CString errorMsg = "Loading requireModule" + ConvertToString(cjsModuleName.GetTaggedValue()) + "failed";
            JSHandle<JSObject> syntaxError =
                factory->GetJSError(base::ErrorType::SYNTAX_ERROR, errorMsg.c_str());
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), JSTaggedValue::Exception());
        }
        return cjsExports.GetTaggedValue();
    }
    return SourceTextModule::GetValueFromExportObject(thread, cjsExports, index);
}

JSTaggedValue ModuleManagerHelper::GetModuleValueFromRecordBinding(JSThread *thread, JSHandle<SourceTextModule> module,
                                                                   JSTaggedValue resolvedBinding)
{
    JSHandle<ResolvedRecordBinding> binding(thread, resolvedBinding);
    JSHandle<JSTaggedValue> moduleRecord(thread, binding->GetModuleRecord());
    ASSERT(moduleRecord->IsString());
    // moduleRecord is string, find at current vm
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> resolvedModule;
    if (moduleManager->IsEvaluatedModule(moduleRecord.GetTaggedValue())) {
        resolvedModule = moduleManager->HostGetImportedModule(moduleRecord.GetTaggedValue());
    } else {
        auto isMergedAbc = !module->GetEcmaModuleRecordName().IsUndefined();
        CString record = ConvertToString(moduleRecord.GetTaggedValue());
        CString fileName = ConvertToString(module->GetEcmaModuleFilename());
        if (!JSPandaFileExecutor::LazyExecuteModule(thread, record, fileName, isMergedAbc)) {
            LOG_ECMA(FATAL) << "LazyExecuteModule failed";
        }
        resolvedModule = moduleManager->HostGetImportedModule(moduleRecord.GetTaggedValue());
    }
    return GetModuleValue(thread, resolvedModule, binding->GetIndex());
}

} // namespace panda::ecmascript
