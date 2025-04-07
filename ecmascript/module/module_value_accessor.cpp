/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "ecmascript/module/module_value_accessor.h"

#include "ecmascript/module/module_logger.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/module/module_path_helper.h"

namespace panda::ecmascript {
JSTaggedValue ModuleValueAccessor::GetModuleValueInner(JSThread *thread, int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule(thread);
    if (currentModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(thread, index, false);
}

JSTaggedValue ModuleValueAccessor::GetModuleValueInner(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(thread, index, false);
}

JSTaggedValue ModuleValueAccessor::GetModuleValueInner(JSThread *thread, int32_t index,
    JSHandle<JSTaggedValue> curModule)
{
    if (curModule->IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(curModule->GetTaggedObject())->GetModuleValue(thread, index, false);
}
JSTaggedValue ModuleValueAccessor::GetSendableModuleValueInner(JSThread* thread, int32_t index, JSTaggedValue jsFunc)
{
    ModuleManager* moduleManager = thread->GetModuleManager();
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetSendableModuleValueInner currentModule is undefined";
    }
    if (!SourceTextModule::IsSendableFunctionModule(currentModule)) {
        return SourceTextModule::Cast(currentModule)->GetModuleValue(thread, index, false);
    }
    CString referenceName = SourceTextModule::GetModuleName(currentModule);
    if (moduleManager->IsModuleLoaded(referenceName)) {
        JSHandle<SourceTextModule> currentModuleHdl = moduleManager->GetImportedModule(referenceName);
        if (currentModuleHdl->GetStatus() > ModuleStatus::INSTANTIATED) {
            return currentModuleHdl->GetModuleValue(thread, index, false);
        }
    }
    JSHandle<SourceTextModule> currentModuleHdl(thread, currentModule);
    auto isMergedAbc = !currentModuleHdl->GetEcmaModuleRecordNameString().empty();
    CString fileName = currentModuleHdl->GetEcmaModuleFilenameString();
    if (!JSPandaFileExecutor::LazyExecuteModule(thread, referenceName, fileName, isMergedAbc)) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "GetSendableModuleValueInner LazyExecuteModule failed";
    }
    ASSERT(moduleManager->IsModuleLoaded(referenceName));
    return moduleManager->GetImportedModule(referenceName)->GetModuleValue(thread, index, false);
}
JSTaggedValue ModuleValueAccessor::GetModuleValueOuter(JSThread* thread, int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule(thread);
    return GetModuleValueOuterInternal(thread, index, currentModule, false);
}

JSTaggedValue ModuleValueAccessor::GetModuleValueOuter(JSThread* thread, int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetModuleValueOuterInternal(thread, index, currentModule, false);
}

JSTaggedValue ModuleValueAccessor::GetModuleValueOuter(JSThread* thread, int32_t index,
    JSHandle<JSTaggedValue> curModule)
{
    return GetModuleValueOuterInternal(thread, index, curModule.GetTaggedValue(), false);
}

JSTaggedValue ModuleValueAccessor::GetLazyModuleValueOuter(JSThread* thread, int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetModuleValueOuterInternal(thread, index, currentModule, true);
}

JSTaggedValue ModuleValueAccessor::GetSendableModuleValueOuter(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetSendableModuleValueOuterInternal(thread, index, currentModule, false);
}

JSTaggedValue ModuleValueAccessor::GetLazySendableModuleValueOuter(JSThread* thread, int32_t index,
    JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetSendableModuleValueOuterInternal(thread, index, currentModule, true);
}

JSTaggedValue ModuleValueAccessor::GetModuleNamespace(JSThread *thread, int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule(thread);
    return GetModuleNamespaceInternal(thread, index, currentModule);
}

JSTaggedValue ModuleValueAccessor::GetModuleNamespace(JSThread *thread, int32_t index, JSTaggedValue curFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(curFunc.GetTaggedObject())->GetModule();
    return GetModuleNamespaceInternal(thread, index, currentModule);
}
JSTaggedValue ModuleValueAccessor::GetNativeOrCjsModuleValue(JSThread *thread, JSHandle<SourceTextModule> module,
    int32_t index)
{
    JSHandle<JSTaggedValue> exports = GetNativeOrCjsExports(thread, module.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    return SourceTextModule::GetValueFromExportObject(thread, exports, index);
}
JSTaggedValue ModuleValueAccessor::GetNativeOrCjsModuleValue(JSThread *thread, JSHandle<SourceTextModule> module,
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
void ModuleValueAccessor::StoreModuleValue(JSThread *thread, int32_t index, JSTaggedValue value)
{
    JSHandle<SourceTextModule> currentModule(thread, GetCurrentModule(thread));
    StoreModuleValueInternal(thread, currentModule, index, value);
}

void ModuleValueAccessor::StoreModuleValue(JSThread *thread, int32_t index, JSTaggedValue value, JSTaggedValue jsFunc)
{
    JSHandle<SourceTextModule> currentModule(thread, JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule());
    StoreModuleValueInternal(thread, currentModule, index, value);
}
JSTaggedValue ModuleValueAccessor::GetCurrentModule(JSThread *thread)
{
    FrameHandler frameHandler(thread);
    JSTaggedValue currentFunc = frameHandler.GetFunction();
    JSTaggedValue module = JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule();
    if (SourceTextModule::IsSendableFunctionModule(module)) {
        CString recordNameStr = SourceTextModule::GetModuleName(module);
        return thread->GetModuleManager()->HostGetImportedModule(recordNameStr).GetTaggedValue();
    }
    return module;
}
JSTaggedValue ModuleValueAccessor::GetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module, int index)
{
    ModuleTypes moduleType = module->GetTypes();
    if (SourceTextModule::IsNativeModule(moduleType) || SourceTextModule::IsCjsModule(moduleType)) {
        return GetNativeOrCjsModuleValue(thread, module, index);
    }
    return module->GetModuleValue(thread, index, false);
}
JSTaggedValue ModuleValueAccessor::GetModuleValueOuterInternal(JSThread *thread, int32_t index,
    JSTaggedValue curModule, bool isLazy)
{
    if (curModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleValueOuterInternal currentModule failed";
        UNREACHABLE();
    }
    JSHandle<SourceTextModule> currentModuleHdl(thread, curModule);
    JSTaggedValue moduleEnvironment = currentModuleHdl->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnvironment.GetTaggedObject())->Get(index);
    GetModuleValueFromBindingInfo info { thread, currentModuleHdl, resolvedBinding, index, false, isLazy };
    if (resolvedBinding.IsResolvedIndexBinding()) {
        return GetModuleValueFromIndexBinding(info);
    }
    if (resolvedBinding.IsResolvedBinding()) {
        return GetModuleValueFromBinding(info);
    }
    if (resolvedBinding.IsResolvedRecordIndexBinding()) {
        return GetModuleValueFromRecordIndexBinding(info);
    }
    if (resolvedBinding.IsResolvedRecordBinding()) { // LCOV_EXCL_BR_LINE
        return GetModuleValueFromRecordBinding(info);
    }
    std::ostringstream oss;
    curModule.Dump(oss);
    LOG_ECMA(FATAL) << "Get module value failed, mistaken ResolvedBinding"
        << ", index: " << index << ", currentModule: " << oss.str();
    UNREACHABLE();
}
JSTaggedValue ModuleValueAccessor::GetSendableModuleValueOuterInternal(JSThread *thread, int32_t index,
    JSTaggedValue curModule, bool isLazy)
{
    if (curModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetSendableModuleValueOuterInternal currentModule failed";
        UNREACHABLE();
    }
    JSHandle<SourceTextModule> module(thread, curModule.GetTaggedObject());
    JSTaggedValue moduleEnvironment = module->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnvironment.GetTaggedObject())->Get(index);
    GetModuleValueFromBindingInfo info { thread, module, resolvedBinding, index, true, isLazy };
    if (resolvedBinding.IsResolvedRecordIndexBinding()) {
        return GetModuleValueFromRecordIndexBinding(info);
    }
    if (resolvedBinding.IsResolvedIndexBinding()) {
        return GetModuleValueFromIndexBinding(info);
    }
    if (resolvedBinding.IsResolvedRecordBinding()) {
        return GetModuleValueFromRecordBinding(info);
    }
    LOG_ECMA(FATAL) << "Unexpect binding";
    UNREACHABLE();
}
JSTaggedValue ModuleValueAccessor::GetModuleNamespaceInternal(JSThread *thread, int32_t index, JSTaggedValue curModule)
{
    if (curModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleNamespace currentModule failed";
        UNREACHABLE();
    }
    JSHandle<SourceTextModule> module(thread, SourceTextModule::Cast(curModule));
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
    JSHandle<SourceTextModule> requiredModule =
        SourceTextModule::GetRequestedModule(thread, requestedModules, index);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());

    ModuleTypes moduleType = requiredModule->GetTypes();
    // if requiredModule is Native module
    if (SourceTextModule::IsNativeModule(moduleType)) {
        LogModuleLoadInfo(thread, module, requiredModule, -1, false);
        return requiredModule->GetModuleValue(thread, 0, false);
    }
    // if requiredModule is CommonJS
    if (SourceTextModule::IsCjsModule(moduleType)) {
        CString cjsModuleName = SourceTextModule::GetModuleName(requiredModule.GetTaggedValue());
        JSHandle<JSTaggedValue> moduleNameHandle(thread->GetEcmaVM()->GetFactory()->NewFromUtf8(cjsModuleName));
        LogModuleLoadInfo(thread, module, requiredModule, -1, false);
        return CjsModule::SearchFromModuleCache(thread, moduleNameHandle).GetTaggedValue();
    }
    // if requiredModule is ESM
    JSHandle<JSTaggedValue> moduleNamespace = SourceTextModule::GetModuleNamespace(thread, requiredModule);
    ASSERT(moduleNamespace->IsModuleNamespace());
    LogModuleLoadInfo(thread, module, requiredModule, -1, false);
    return moduleNamespace.GetTaggedValue();
}
void ModuleValueAccessor::StoreModuleValueInternal(JSThread *thread, JSHandle<SourceTextModule> &curModule,
    int32_t index, JSTaggedValue value)
{
    if (curModule.GetTaggedValue().IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "StoreModuleValue currentModule failed";
        UNREACHABLE();
    }
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    SourceTextModule::StoreModuleValue(thread, curModule, index, valueHandle);
}

JSTaggedValue ModuleValueAccessor::GetModuleValueFromIndexBinding(const GetModuleValueFromBindingInfo &info)
{
    JSHandle<ResolvedIndexBinding> binding(info.thread, info.resolvedBinding);
    JSMutableHandle<SourceTextModule> resolvedModule(info.thread, binding->GetModule());
    if (!info.isSendable && info.thread->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN) {
        const JSHandle<JSTaggedValue> resolvedModuleOfHotReload =
            info.thread->GetEcmaVM()->FindPatchModule(resolvedModule->GetEcmaModuleRecordNameString());
        if (!resolvedModuleOfHotReload->IsHole()) {
            resolvedModule.Update(resolvedModuleOfHotReload);
        }
    }
    if (info.isSendable && info.isLazy) { // bug need fix
        SourceTextModule::Evaluate(info.thread, resolvedModule);
    } else {
        EvaluateModuleIfNeeded(info.thread, resolvedModule, info.isLazy);
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(info.thread, JSTaggedValue::Exception());
    LogModuleLoadInfo(info.thread, info.module, resolvedModule, info.index, info.isSendable);
    return GetModuleValue(info.thread, resolvedModule, binding->GetIndex());
}

JSTaggedValue ModuleValueAccessor::GetModuleValueFromBinding(const GetModuleValueFromBindingInfo &info)
{
    JSHandle<ResolvedBinding> binding(info.thread, info.resolvedBinding);
    JSHandle<SourceTextModule> resolvedModule(info.thread, binding->GetModule());
    if (UNLIKELY(!SourceTextModule::IsNativeModule(resolvedModule->GetTypes()) &&
        !SourceTextModule::IsCjsModule(resolvedModule->GetTypes()))) {
        std::ostringstream oss;
        info.module.GetTaggedValue().Dump(oss);
        LOG_ECMA(FATAL) << "Get module value failed, mistaken ResolvedBinding"
            << ", index: " << info.index << ", currentModule: " << oss.str();
    }
    EvaluateModuleIfNeeded(info.thread, resolvedModule, info.isLazy);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(info.thread, JSTaggedValue::Exception());
    LogModuleLoadInfo(info.thread, info.module, resolvedModule, info.index, false);
    return UpdateBindingAndGetModuleValue(info.thread, info.module, resolvedModule, info.index,
        binding->GetBindingName());
}

JSTaggedValue ModuleValueAccessor::GetModuleValueFromRecordIndexBinding(const GetModuleValueFromBindingInfo &info)
{
    JSHandle<ResolvedRecordIndexBinding> binding(info.thread, info.resolvedBinding);
    JSHandle<SourceTextModule> resolvedModule =
        GetResolvedModuleFromRecordIndexBinding(info.thread, info.module, binding, info.isLazy);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(info.thread, JSTaggedValue::Exception());
    LogModuleLoadInfo(info.thread, info.module, resolvedModule, info.index, info.isSendable);
    return GetModuleValue(info.thread, resolvedModule, binding->GetIndex());
}

JSTaggedValue ModuleValueAccessor::GetModuleValueFromRecordBinding(const GetModuleValueFromBindingInfo &info)
{
    JSHandle<ResolvedRecordBinding> binding(info.thread, info.resolvedBinding);
    JSHandle<SourceTextModule> resolvedModule =
        GetResolvedModuleFromRecordBinding(info.thread, info.module, binding, info.isLazy);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(info.thread, JSTaggedValue::Exception());
    LogModuleLoadInfo(info.thread, info.module, resolvedModule, info.index, info.isSendable);
    return GetNativeOrCjsModuleValue(info.thread, resolvedModule, binding->GetBindingName());
}

JSTaggedValue ModuleValueAccessor::UpdateBindingAndGetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module,
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
        CString requestMod = ModulePathHelper::ReformatPath(SourceTextModule::GetModuleName(
            requiredModule.GetTaggedValue()));
        CString recordStr = ModulePathHelper::ReformatPath(SourceTextModule::GetModuleName(
            module.GetTaggedValue()));
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

JSHandle<SourceTextModule> ModuleValueAccessor::GetResolvedModuleFromRecordIndexBinding(JSThread* thread,
    JSHandle<SourceTextModule> module, JSHandle<ResolvedRecordIndexBinding> binding, bool isLazy)
{
    return GetResolvedModule(thread, module,
        ModulePathHelper::Utf8ConvertToString(binding->GetAbcFileName()),
        ModulePathHelper::Utf8ConvertToString(binding->GetModuleRecord()), isLazy);
}

JSHandle<SourceTextModule> ModuleValueAccessor::GetResolvedModuleFromRecordBinding(JSThread* thread,
    JSHandle<SourceTextModule> module, JSHandle<ResolvedRecordBinding> binding, bool isLazy)
{
    return GetResolvedModule(thread, module, module->GetEcmaModuleFilenameString(),
        ModulePathHelper::Utf8ConvertToString(binding->GetModuleRecord()), isLazy);
}

void ModuleValueAccessor::EvaluateModuleIfNeeded(JSThread* thread, JSHandle<SourceTextModule> module, bool isLazy)
{
    if  (!isLazy) {
        return;
    }
    if (module->GetStatus() >= ModuleStatus::EVALUATED) {
        return;
    }
    const ModuleTypes moduleType = module->GetTypes();
    if (SourceTextModule::IsNativeModule(moduleType)) {
        SourceTextModule::EvaluateNativeModule(thread, module, moduleType);
        return;
    }
    SourceTextModule::Evaluate(thread, module, nullptr, 0, ExecuteTypes::LAZY);
}

void ModuleValueAccessor::LogModuleLoadInfo(JSThread* thread, JSHandle<SourceTextModule> module,
    JSHandle<SourceTextModule> requiredModule, int32_t index, bool isSendable)
{
    if (isSendable) {
        return;
    }
    ModuleLogger *moduleLogger = thread->GetModuleLogger();
    if (moduleLogger != nullptr) {
        moduleLogger->InsertModuleLoadInfo(module, requiredModule, index);
    }
}
JSHandle<JSTaggedValue> ModuleValueAccessor::GetNativeOrCjsExports(JSThread *thread, JSTaggedValue resolvedModule)
{
    JSHandle<SourceTextModule> module(thread, resolvedModule);
    // if cjsModule is not JSObject, means cjs uses default exports.
    JSMutableHandle<JSTaggedValue> exports(thread, thread->GlobalConstants()->GetUndefined());
    ModuleTypes moduleType = module->GetTypes();
    if (SourceTextModule::IsNativeModule(moduleType)) {
        exports.Update(module->GetModuleValue(thread, 0, false));
        if (!exports->IsJSObject()) {
            LOG_FULL(WARN) << "Load native module failed, so is " << SourceTextModule::GetModuleName(resolvedModule);
        }
        return exports;
    }
    if (SourceTextModule::IsCjsModule(moduleType)) {
        CString cjsModuleName = SourceTextModule::GetModuleName(module.GetTaggedValue());
        JSHandle<JSTaggedValue> moduleNameHandle(thread->GetEcmaVM()->GetFactory()->NewFromUtf8(cjsModuleName));
        exports.Update(CjsModule::SearchFromModuleCache(thread, moduleNameHandle).GetTaggedValue());
        if (exports->IsHole()) {
            CString errorMsg =
                "Loading cjs module:" + SourceTextModule::GetModuleName(module.GetTaggedValue()) + ", failed";
            JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
            THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread,
                ErrorType::SYNTAX_ERROR, errorMsg.c_str(), exception);
        }
        return exports;
    }
    return exports;
}

JSHandle<SourceTextModule> ModuleValueAccessor::GetResolvedModule(JSThread* thread, JSHandle<SourceTextModule> module,
    const CString& fileName, const CString& requestModuleRecordName, bool isLazy)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    const JSHandle<JSTaggedValue> cachedModule = moduleManager->TryGetImportedModule(requestModuleRecordName);
    if (!cachedModule->IsUndefined()) {
        JSHandle<SourceTextModule> resolvedModule = JSHandle<SourceTextModule>::Cast(cachedModule);
        if (resolvedModule->GetStatus() >= ModuleStatus::EVALUATED) {
            return resolvedModule;
        }
        if (isLazy) {
            EvaluateModuleIfNeeded(thread, resolvedModule, isLazy);
            return resolvedModule;
        }
    }
    bool isMergedAbc = !module->GetEcmaModuleFilenameString().empty();
    if (!JSPandaFileExecutor::LazyExecuteModule(thread, requestModuleRecordName, fileName,
        isMergedAbc)) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "LazyExecuteModule failed";
    }
    return moduleManager->HostGetImportedModule(requestModuleRecordName);
}

JSTaggedValue DeprecatedModuleValueAccessor::GetModuleValueInner(JSThread* thread, JSTaggedValue key)
{
    JSTaggedValue currentModule = ModuleValueAccessor::GetCurrentModule(thread);
    if (currentModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
        UNREACHABLE();
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(thread, key, false);
}

JSTaggedValue DeprecatedModuleValueAccessor::GetModuleValueInner(JSThread* thread, JSTaggedValue key,
    JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
        UNREACHABLE();
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(thread, key, false);
}

JSTaggedValue DeprecatedModuleValueAccessor::GetModuleValueOuter(JSThread* thread, JSTaggedValue key)
{
    JSTaggedValue currentModule = ModuleValueAccessor::GetCurrentModule(thread);
    return GetModuleValueOuterInternal(thread, key, currentModule);
}

JSTaggedValue DeprecatedModuleValueAccessor::GetModuleValueOuter(JSThread* thread, JSTaggedValue key,
    JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetModuleValueOuterInternal(thread, key, currentModule);
}

JSTaggedValue DeprecatedModuleValueAccessor::GetModuleNamespace(JSThread* thread, JSTaggedValue localName)
{
    JSTaggedValue currentModule = ModuleValueAccessor::GetCurrentModule(thread);
    return GetModuleNamespaceInternal(thread, localName, currentModule);
}

JSTaggedValue DeprecatedModuleValueAccessor::GetModuleNamespace(JSThread* thread, JSTaggedValue localName,
    JSTaggedValue curFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(curFunc.GetTaggedObject())->GetModule();
    return GetModuleNamespaceInternal(thread, localName, currentModule);
}
void DeprecatedModuleValueAccessor::StoreModuleValue(JSThread* thread, JSTaggedValue key, JSTaggedValue value)
{
    JSHandle<SourceTextModule> currentModule(thread, ModuleValueAccessor::GetCurrentModule(thread));
    StoreModuleValueInternal(thread, currentModule, key, value);
}

void DeprecatedModuleValueAccessor::StoreModuleValue(JSThread* thread, JSTaggedValue key, JSTaggedValue value,
    JSTaggedValue jsFunc)
{
    JSHandle<SourceTextModule> currentModule(thread, JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule());
    StoreModuleValueInternal(thread, currentModule, key, value);
}
JSTaggedValue DeprecatedModuleValueAccessor::GetModuleValueOuterInternal(JSThread* thread, JSTaggedValue key,
    JSTaggedValue curModule)
{
    if (curModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleValueOutter currentModule failed";
        UNREACHABLE();
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(curModule.GetTaggedObject())->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    int entry = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->FindEntry(key);
    if (entry == -1) {
        return thread->GlobalConstants()->GetUndefined();
    }
    JSTaggedValue resolvedBinding = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->GetValue(entry);
    ASSERT(resolvedBinding.IsResolvedBinding());
    ResolvedBinding *binding = ResolvedBinding::Cast(resolvedBinding.GetTaggedObject());
    JSTaggedValue resolvedModule = binding->GetModule();
    ASSERT(resolvedModule.IsSourceTextModule());
    SourceTextModule *module = SourceTextModule::Cast(resolvedModule.GetTaggedObject());
    if (module->GetTypes() == ModuleTypes::CJS_MODULE) {
        CString cjsModuleName = SourceTextModule::GetModuleName(JSTaggedValue(module));
        JSHandle<JSTaggedValue> moduleNameHandle(thread->GetEcmaVM()->GetFactory()->NewFromUtf8(cjsModuleName));
        return CjsModule::SearchFromModuleCache(thread, moduleNameHandle).GetTaggedValue();
    }
    return module->GetModuleValue(thread, binding->GetBindingName(), false);
}
JSTaggedValue DeprecatedModuleValueAccessor::GetModuleNamespaceInternal(JSThread* thread, JSTaggedValue localName,
    JSTaggedValue curModule)
{
    if (curModule.IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "GetModuleNamespace currentModule failed";
        UNREACHABLE();
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(curModule.GetTaggedObject())->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    int entry = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->FindEntry(localName);
    if (entry == -1) {
        return thread->GlobalConstants()->GetUndefined();
    }
    JSTaggedValue moduleNamespace = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->GetValue(entry);
    ASSERT(moduleNamespace.IsModuleNamespace());
    return moduleNamespace;
}

void DeprecatedModuleValueAccessor::StoreModuleValueInternal(JSThread* thread, JSHandle<SourceTextModule> &curModule,
    JSTaggedValue key, JSTaggedValue value)
{
    if (curModule.GetTaggedValue().IsUndefined()) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "StoreModuleValue currentModule failed";
        UNREACHABLE();
    }
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    SourceTextModule::StoreModuleValue(thread, curModule, keyHandle, valueHandle);
}
} // namespace panda::ecmascript
