/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/module/js_module_source_text.h"

#include "ecmascript/global_env.h"
#include "ecmascript/base/path_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;
using StringHelper = base::StringHelper;

CVector<std::string> SourceTextModule::GetExportedNames(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                                        const JSHandle<TaggedArray> &exportStarSet)
{
    CVector<std::string> exportedNames;
    // 1. Let module be this Source Text Module Record.
    // 2. If exportStarSet contains module, then
    if (exportStarSet->GetIdx(module.GetTaggedValue()) != TaggedArray::MAX_ARRAY_INDEX) {
        // a. Assert: We've reached the starting point of an import * circularity.
        // b. Return a new empty List.
        return exportedNames;
    }
    // 3. Append module to exportStarSet.
    size_t len = exportStarSet->GetLength();
    JSHandle<TaggedArray> newExportStarSet = TaggedArray::SetCapacity(thread, exportStarSet, len + 1);
    newExportStarSet->Set(thread, len, module.GetTaggedValue());

    JSTaggedValue entryValue = module->GetLocalExportEntries();
    // 5. For each ExportEntry Record e in module.[[LocalExportEntries]], do
    AddExportName<LocalExportEntry>(thread, entryValue, exportedNames);

    // 6. For each ExportEntry Record e in module.[[IndirectExportEntries]], do
    entryValue = module->GetIndirectExportEntries();
    AddExportName<IndirectExportEntry>(thread, entryValue, exportedNames);

    entryValue = module->GetStarExportEntries();
    auto globalConstants = thread->GlobalConstants();
    if (!entryValue.IsUndefined()) {
        JSMutableHandle<StarExportEntry> ee(thread, globalConstants->GetUndefined());
        JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());

        // 7. For each ExportEntry Record e in module.[[StarExportEntries]], do
        JSHandle<TaggedArray> starExportEntries(thread, entryValue);
        size_t starExportEntriesLen = starExportEntries->GetLength();
        for (size_t idx = 0; idx < starExportEntriesLen; idx++) {
            ee.Update(starExportEntries->Get(idx));
            // a. Let requestedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
            moduleRequest.Update(ee->GetModuleRequest());
            SetExportName(thread, moduleRequest, module, exportedNames, newExportStarSet);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exportedNames);
        }
    }
    return exportedNames;
}

// new way with module
JSHandle<JSTaggedValue> SourceTextModule::HostResolveImportedModuleWithMerge(
    JSThread *thread, const JSHandle<SourceTextModule> &module, const JSHandle<JSTaggedValue> &moduleRequest)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    if (moduleManager->IsImportedModuleLoaded(moduleRequest.GetTaggedValue())) {
        return JSHandle<JSTaggedValue>(moduleManager->HostGetImportedModule(moduleRequest.GetTaggedValue()));
    }

    CString moduleRequestName = ConvertToString(EcmaString::Cast(moduleRequest->GetTaggedObject()));
    auto [isNative, moduleType] = SourceTextModule::CheckNativeModule(moduleRequestName);
    if (isNative) {
        return moduleManager->ResolveNativeModule(moduleRequestName, moduleType);
    }

    CString baseFilename = ConvertToString(module->GetEcmaModuleFilename());
    CString moduleRecordName = ConvertToString(module->GetEcmaModuleRecordName());
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFilename, moduleRecordName);
    if (jsPandaFile == nullptr) {
        CString msg = "Load file with filename '" + baseFilename + "' failed, recordName '" + moduleRecordName + "'";
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }

    CString outFileName = baseFilename;
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(
        thread, jsPandaFile.get(), outFileName, moduleRecordName, moduleRequestName);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

#if defined(PANDA_TARGET_WINDOWS) || defined(PANDA_TARGET_MACOS)
    if (entryPoint == ModulePathHelper::PREVIEW_OF_ACROSS_HAP_FLAG &&
        thread->GetEcmaVM()->EnableReportModuleResolvingFailure()) {
        THROW_SYNTAX_ERROR_AND_RETURN(thread, "", thread->GlobalConstants()->GetHandledUndefined());
    }
#endif
    return moduleManager->HostResolveImportedModuleWithMerge(outFileName, entryPoint);
}

// old way with bundle
JSHandle<JSTaggedValue> SourceTextModule::HostResolveImportedModule(JSThread *thread,
                                                                    const JSHandle<SourceTextModule> &module,
                                                                    const JSHandle<JSTaggedValue> &moduleRequest)
{
    auto moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    if (moduleManager->IsImportedModuleLoaded(moduleRequest.GetTaggedValue())) {
        return JSHandle<JSTaggedValue>(moduleManager->HostGetImportedModule(moduleRequest.GetTaggedValue()));
    }

    JSHandle<EcmaString> dirname = base::PathHelper::ResolveDirPath(thread,
        ConvertToString(module->GetEcmaModuleFilename()));
    JSHandle<EcmaString> moduleFilename = ResolveFilenameFromNative(thread, dirname.GetTaggedValue(),
        moduleRequest.GetTaggedValue());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return thread->GetCurrentEcmaContext()->GetModuleManager()->
        HostResolveImportedModule(ConvertToString(moduleFilename.GetTaggedValue()));
}

bool SourceTextModule::CheckCircularImport(const JSHandle<SourceTextModule> &module,
    const JSHandle<JSTaggedValue> &exportName,
    CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> &resolveVector)
{
    for (auto rr : resolveVector) {
        // a. If module and r.[[Module]] are the same Module Record and
        // SameValue(exportName, r.[[ExportName]]) is true, then
        if (JSTaggedValue::SameValue(rr.first.GetTaggedValue(), module.GetTaggedValue()) &&
            JSTaggedValue::SameValue(rr.second, exportName)) {
            // i. Assert: This is a circular import request.
            // ii. Return true.
            return true;
        }
    }
    return false;
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveExportObject(JSThread *thread,
                                                              const JSHandle<SourceTextModule> &module,
                                                              const JSHandle<JSTaggedValue> &exports,
                                                              const JSHandle<JSTaggedValue> &exportName)
{
    // Let module be this Source Text Module Record.
    auto globalConstants = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // For CJS, if exports is not JSObject, means the CJS module use default output
    JSHandle<JSTaggedValue> defaultString = globalConstants->GetHandledDefaultString();
    if (JSTaggedValue::SameValue(exportName, defaultString)) {
        // bind with a number
        return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedIndexBindingRecord(module, -1));
    }
    if (exports->IsJSObject()) {
        JSHandle<JSTaggedValue> resolution(thread, JSTaggedValue::Hole());
        JSObject *exportObject = JSObject::Cast(exports.GetTaggedValue().GetTaggedObject());
        TaggedArray *properties = TaggedArray::Cast(exportObject->GetProperties().GetTaggedObject());
        if (!properties->IsDictionaryMode()) {
            JSHandle<JSHClass> jsHclass(thread, exportObject->GetJSHClass());
            // Get layoutInfo and compare the input and output names of files
            LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
            if (layoutInfo->NumberOfElements() != 0) {
                resolution = ResolveElementOfObject(thread, jsHclass, exportName, module);
            }
        } else {
            NameDictionary *dict = NameDictionary::Cast(properties);
            int entry = dict->FindEntry(exportName.GetTaggedValue());
            if (entry != -1) {
                resolution = JSHandle<JSTaggedValue>::Cast(factory->NewResolvedIndexBindingRecord(module, entry));
            }
        }
        if (!resolution->IsUndefined()) {
            return resolution;
        }
    }
    return globalConstants->GetHandledNull();
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveExport(JSThread *thread, const JSHandle<SourceTextModule> &module,
    const JSHandle<JSTaggedValue> &exportName,
    CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> &resolveVector)
{
    // 1. Let module be this Source Text Module Record.
    auto globalConstants = thread->GlobalConstants();
    // Check if circular import request.
    // 2.For each Record { [[Module]], [[ExportName]] } r in resolveVector, do
    if (CheckCircularImport(module, exportName, resolveVector)) {
        return globalConstants->GetHandledNull();
    }
    // 3. Append the Record { [[Module]]: module, [[ExportName]]: exportName } to resolveVector.
    resolveVector.emplace_back(std::make_pair(module, exportName));
    // 4. For each ExportEntry Record e in module.[[LocalExportEntries]], do
    JSHandle<JSTaggedValue> localExportEntriesTv(thread, module->GetLocalExportEntries());
    if (!localExportEntriesTv->IsUndefined()) {
        JSHandle<JSTaggedValue> resolution = ResolveLocalExport(thread, localExportEntriesTv, exportName, module);
        if (!resolution->IsUndefined()) {
            return resolution;
        }
    }
    // 5. For each ExportEntry Record e in module.[[IndirectExportEntries]], do
    JSHandle<JSTaggedValue> indirectExportEntriesTv(thread, module->GetIndirectExportEntries());
    if (!indirectExportEntriesTv->IsUndefined()) {
        JSHandle<JSTaggedValue> resolution = ResolveIndirectExport(thread, indirectExportEntriesTv,
                                                                   exportName, module, resolveVector);
        if (!resolution->IsUndefined()) {
            return resolution;
        }
    }
    // 6. If SameValue(exportName, "default") is true, then
    JSHandle<JSTaggedValue> defaultString = globalConstants->GetHandledDefaultString();
    // In Aot static parse phase, some importModule maybe empty aot module, all elements will be undefined, it will
    // return hole for resolve index binding at the end to skip error.
    if (JSTaggedValue::SameValue(exportName, defaultString) &&
        thread->GetEcmaVM()->EnableReportModuleResolvingFailure()) {
        // a. Assert: A default export was not explicitly defined by this module.
        // b. Return null.
        // c. NOTE: A default export cannot be provided by an export *.
        return globalConstants->GetHandledNull();
    }
    // 7. Let starResolution be null.
    JSMutableHandle<JSTaggedValue> starResolution(thread, globalConstants->GetNull());
    // 8. For each ExportEntry Record e in module.[[StarExportEntries]], do
    JSTaggedValue starExportEntriesTv = module->GetStarExportEntries();
    if (starExportEntriesTv.IsUndefined()) {
        // return Hole in Aot static parse phase to skip error.
        if (!thread->GetEcmaVM()->EnableReportModuleResolvingFailure()) {
            starResolution.Update(JSTaggedValue::Hole());
        }
        return starResolution;
    }
    JSMutableHandle<StarExportEntry> ee(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    JSHandle<TaggedArray> starExportEntries(thread, starExportEntriesTv);
    size_t starExportEntriesLen = starExportEntries->GetLength();
    for (size_t idx = 0; idx < starExportEntriesLen; idx++) {
        ee.Update(starExportEntries->Get(idx));
        moduleRequest.Update(ee->GetModuleRequest());
        JSHandle<JSTaggedValue> result = GetStarResolution(thread, exportName, moduleRequest,
                                                           module, starResolution, resolveVector);
        if (result->IsString() || result->IsException()) {
            return result;
        }
    }
    // 9. Return starResolution.
    return starResolution;
}

void SourceTextModule::InstantiateCJS(JSThread *thread, const JSHandle<SourceTextModule> &currentModule,
                                      const JSHandle<SourceTextModule> &requiredModule)
{
    JSTaggedValue cjsFileName(requiredModule->GetEcmaModuleFilename());
    JSTaggedValue cjsRecordName(requiredModule->GetEcmaModuleRecordName());
    JSMutableHandle<JSTaggedValue> cjsModuleName(thread, JSTaggedValue::Undefined());
    // Get exported cjs module
    if (cjsRecordName.IsUndefined()) {
        cjsModuleName.Update(cjsFileName);
    } else {
        cjsModuleName.Update(cjsRecordName);
    }
    JSHandle<JSTaggedValue> cjsExports = CjsModule::SearchFromModuleCache(thread, cjsModuleName);
    InitializeEnvironment(thread, currentModule, cjsModuleName, cjsExports);
}

std::pair<bool, ModuleTypes> SourceTextModule::CheckNativeModule(const CString &moduleRequestName)
{
    if (moduleRequestName[0] != '@' ||
        StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::PREFIX_BUNDLE) ||
        StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::PREFIX_PACKAGE)||
        moduleRequestName.find(':') == CString::npos) {
        return {false, ModuleTypes::UNKNOWN};
    }

    if (StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::REQUIRE_NAPI_OHOS_PREFIX)) {
        return {true, ModuleTypes::OHOS_MODULE};
    }
    if (StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::REQUIRE_NAPI_APP_PREFIX)) {
        return {true, ModuleTypes::APP_MODULE};
    }
    if (StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::REQUIRE_NAITVE_MODULE_PREFIX)) {
        return {true, ModuleTypes::NATIVE_MODULE};
    }
    return {true, ModuleTypes::INTERNAL_MODULE};
}

Local<JSValueRef> SourceTextModule::GetRequireNativeModuleFunc(EcmaVM *vm, ModuleTypes moduleType)
{
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm);
    auto globalConstants = vm->GetJSThread()->GlobalConstants();
    auto funcName = (moduleType == ModuleTypes::NATIVE_MODULE) ?
        globalConstants->GetHandledRequireNativeModuleString() :
        globalConstants->GetHandledRequireNapiString();
    return globalObject->Get(vm, JSNApiHelper::ToLocal<StringRef>(funcName));
}
   
void SourceTextModule::MakeAppArgs(const EcmaVM *vm, std::vector<Local<JSValueRef>> &arguments,
    const CString &moduleName)
{
    size_t pos = moduleName.find_last_of('/');
    if (pos == CString::npos) {
        LOG_FULL(FATAL) << "Invalid native module " << moduleName;
        UNREACHABLE();
    }
    CString soName = moduleName.substr(pos + 1);
    CString path = moduleName.substr(0, pos);
    // use module name as so name
    arguments[0] = StringRef::NewFromUtf8(vm, soName.c_str());
    arguments.emplace_back(BooleanRef::New(vm, true));
    arguments.emplace_back(StringRef::NewFromUtf8(vm, path.c_str()));
}

void SourceTextModule::MakeInternalArgs(const EcmaVM *vm, std::vector<Local<JSValueRef>> &arguments,
    const CString &moduleRequestName)
{
    arguments.emplace_back(BooleanRef::New(vm, false));
    arguments.emplace_back(StringRef::NewFromUtf8(vm, ""));
    CString moduleDir = PathHelper::GetInternalModulePrefix(moduleRequestName);
    arguments.emplace_back(StringRef::NewFromUtf8(vm, moduleDir.c_str()));
}

bool SourceTextModule::LoadNativeModule(JSThread *thread, JSHandle<SourceTextModule> &requiredModule,
    const JSHandle<JSTaggedValue> &moduleRequest, ModuleTypes moduleType)
{
    EcmaVM *vm = thread->GetEcmaVM();
    [[maybe_unused]] LocalScope scope(vm);

    CString moduleRequestName = ConvertToString(EcmaString::Cast(moduleRequest->GetTaggedObject()));
    CString moduleName = PathHelper::GetStrippedModuleName(moduleRequestName);
    std::vector<Local<JSValueRef>> arguments;
    LOG_FULL(DEBUG) << "Request module is " << moduleRequestName;

    arguments.emplace_back(StringRef::NewFromUtf8(vm, moduleName.c_str()));
    if (moduleType == ModuleTypes::APP_MODULE) {
        MakeAppArgs(vm, arguments, moduleName);
    } else if (moduleType == ModuleTypes::INTERNAL_MODULE) {
        MakeInternalArgs(vm, arguments, moduleRequestName);
    }
    auto maybeFuncRef = GetRequireNativeModuleFunc(vm, moduleType);
    // some function(s) may not registered in global object for non-main thread
    if (!maybeFuncRef->IsFunction()) {
        LOG_FULL(WARN) << "Not found require func";
        return false;
    }

    Local<FunctionRef> funcRef = maybeFuncRef;
    auto exportObject = funcRef->Call(vm, JSValueRef::Undefined(vm), arguments.data(), arguments.size());
    if (UNLIKELY(thread->HasPendingException())) {
        thread->ClearException();
        LOG_FULL(ERROR) << "LoadNativeModule has exception";
        return false;
    }
    requiredModule->StoreModuleValue(thread, 0, JSNApiHelper::ToJSHandle(exportObject));
    return true;
}

void SourceTextModule::InstantiateNativeModule(JSThread *thread, JSHandle<SourceTextModule> &currentModule,
    JSHandle<SourceTextModule> &requiredModule, const JSHandle<JSTaggedValue> &moduleRequest,
    ModuleTypes moduleType)
{
    if (requiredModule->GetStatus() != ModuleStatus::EVALUATED) {
        if (!SourceTextModule::LoadNativeModule(thread, requiredModule, moduleRequest, moduleType)) {
            LOG_FULL(WARN) << "LoadNativeModule " << ConvertToString(
                EcmaString::Cast(moduleRequest->GetTaggedObject())) << " failed";
            return;
        }
    }

    JSHandle<JSTaggedValue> nativeModuleName(thread, requiredModule->GetEcmaModuleRecordName());
    JSHandle<JSTaggedValue> nativeExports(thread, requiredModule->GetModuleValue(thread, 0, false));
    InitializeEnvironment(thread, currentModule, nativeModuleName, nativeExports);
}

void SourceTextModule::InitializeEnvironment(JSThread *thread, const JSHandle<SourceTextModule> &currentModule,
    JSHandle<JSTaggedValue> &moduleName, JSHandle<JSTaggedValue> &exports)
{
    // Get esm environment
    JSHandle<JSTaggedValue> moduleEnvironment(thread, currentModule->GetEnvironment());
    auto globalConstants = thread->GlobalConstants();
    if (moduleEnvironment->IsUndefined()) {
        return;
    }
    JSHandle<TaggedArray> environment = JSHandle<TaggedArray>::Cast(moduleEnvironment);
    size_t length = environment->GetLength();
    JSHandle<TaggedArray> importEntries(thread, currentModule->GetImportEntries());
    JSMutableHandle<ImportEntry> host(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    // update required module
    for (size_t idx = 0; idx < length; idx++) {
        JSTaggedValue resolvedBinding = environment->Get(idx);
        // if resolvedBinding.IsHole(), means that importname is * .
        if (resolvedBinding.IsHole()) {
            continue;
        }
        JSHandle<SourceTextModule> requestedModule = GetModuleFromBinding(thread, resolvedBinding);
        JSMutableHandle<JSTaggedValue> requestedName(thread, JSTaggedValue::Undefined());
        JSTaggedValue recordName = requestedModule->GetEcmaModuleRecordName();
        if (recordName.IsUndefined()) {
            requestedName.Update(requestedModule->GetEcmaModuleFilename());
        } else {
            requestedName.Update(recordName);
        }
        // if not the same module, then don't have to update
        if (!JSTaggedValue::SameValue(requestedName, moduleName)) {
            continue;
        }
        // rebinding here
        host.Update(importEntries->Get(idx));
        importName.Update(host->GetImportName());
        JSHandle<JSTaggedValue> resolution =
            SourceTextModule::ResolveExportObject(thread, requestedModule, exports, importName);
        // ii. If resolution is null or "ambiguous", throw a SyntaxError exception.
        if (resolution->IsNull() || resolution->IsString()) {
            CString msg = "the requested module '" +
                          ConvertToString(host->GetModuleRequest()) +
                          "' does not provide an export named '" +
                          ConvertToString(importName.GetTaggedValue()) +
                          "' which imported by '" +
                          ConvertToString(currentModule->GetEcmaModuleRecordName()) + "'";
            THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
        }
        // iii. Call envRec.CreateImportBinding(
        // in.[[LocalName]], resolution.[[Module]], resolution.[[BindingName]]).
        environment->Set(thread, idx, resolution);
    }
}

JSHandle<SourceTextModule> SourceTextModule::GetModuleFromBinding(JSThread *thread,
                                                                  const JSTaggedValue &resolvedBinding)
{
    if (resolvedBinding.IsResolvedIndexBinding()) {
        ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
        return JSHandle<SourceTextModule>(thread, binding->GetModule());
    }
    ResolvedBinding *binding = ResolvedBinding::Cast(resolvedBinding.GetTaggedObject());
    return JSHandle<SourceTextModule>(thread, binding->GetModule());
}

int SourceTextModule::Instantiate(JSThread *thread, const JSHandle<JSTaggedValue> &moduleHdl,
    bool excuteFromJob)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SourceTextModule::Instantiate");
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, SourceTextModule::UNDEFINED_INDEX);
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleHdl);
    // 1. Let module be this Source Text Module Record.
    // 2. Assert: module.[[Status]] is not "instantiating" or "evaluating".
    ASSERT(module->GetStatus() != ModuleStatus::INSTANTIATING && module->GetStatus() != ModuleStatus::EVALUATING);
    // 3. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    // 4. Let result be InnerModuleInstantiation(module, stack, 0).
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    int result = SourceTextModule::InnerModuleInstantiation(thread, moduleRecord, stack, 0, excuteFromJob);
    // 5. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        // a. For each module m in stack, do
        for (auto mm : stack) {
            // i. Assert: m.[[Status]] is "instantiating".
            ASSERT(mm->GetStatus() == ModuleStatus::INSTANTIATING);
            // ii. Set m.[[Status]] to "uninstantiated".
            mm->SetStatus(ModuleStatus::UNINSTANTIATED);
            // iii. Set m.[[Environment]] to undefined.
            // iv. Set m.[[DFSIndex]] to undefined.
            mm->SetDFSIndex(SourceTextModule::UNDEFINED_INDEX);
            // v. Set m.[[DFSAncestorIndex]] to undefined.
            mm->SetDFSAncestorIndex(SourceTextModule::UNDEFINED_INDEX);
        }
        // b. Assert: module.[[Status]] is "uninstantiated".
        ASSERT(module->GetStatus() == ModuleStatus::UNINSTANTIATED);
        // c. return result
        return result;
    }
    // 6. Assert: module.[[Status]] is "instantiated" or "evaluated".
    ASSERT(module->GetStatus() == ModuleStatus::INSTANTIATED || module->GetStatus() == ModuleStatus::EVALUATED);
    // 7. Assert: stack is empty.
    ASSERT(stack.empty());
    // 8. Return undefined.
    return SourceTextModule::UNDEFINED_INDEX;
}

int SourceTextModule::InnerModuleInstantiation(JSThread *thread, const JSHandle<ModuleRecord> &moduleRecord,
    CVector<JSHandle<SourceTextModule>> &stack, int index, bool excuteFromJob)
{
    // 1. If module is not a Source Text Module Record, then
    if (!moduleRecord.GetTaggedValue().IsSourceTextModule()) {
        //  a. Perform ? module.Instantiate().
        ModuleRecord::Instantiate(thread, JSHandle<JSTaggedValue>::Cast(moduleRecord));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        //  b. Return index.
        return index;
    }
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    // 2. If module.[[Status]] is "instantiating", "instantiated", or "evaluated", then Return index.
    ModuleStatus status = module->GetStatus();
    if (status == ModuleStatus::INSTANTIATING ||
        status == ModuleStatus::INSTANTIATED ||
        status == ModuleStatus::EVALUATED) {
        return index;
    }
    // 3. Assert: module.[[Status]] is "uninstantiated".
    ASSERT(status == ModuleStatus::UNINSTANTIATED);
    // 4. Set module.[[Status]] to "instantiating".
    module->SetStatus(ModuleStatus::INSTANTIATING);
    // 5. Set module.[[DFSIndex]] to index.
    module->SetDFSIndex(index);
    // 6. Set module.[[DFSAncestorIndex]] to index.
    module->SetDFSAncestorIndex(index);
    // 7. Set index to index + 1.
    index++;
    // 8. Append module to stack.
    stack.emplace_back(module);
    // 9. For each String required that is an element of module.[[RequestedModules]], do
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            required.Update(requestedModules->Get(idx));
            // a. Let requiredModule be ? HostResolveImportedModule(module, required).
            JSMutableHandle<SourceTextModule> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
            JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
            if (moduleRecordName.IsUndefined()) {
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModule(thread, module, required);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, SourceTextModule::UNDEFINED_INDEX);
                ModuleDeregister::InitForDeregisterModule(thread, requiredVal, excuteFromJob);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
            } else {
                ASSERT(moduleRecordName.IsString());
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, required);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, SourceTextModule::UNDEFINED_INDEX);
                ModuleDeregister::InitForDeregisterModule(thread, requiredVal, excuteFromJob);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
            }

            // b. Set index to ? InnerModuleInstantiation(requiredModule, stack, index).
            JSHandle<ModuleRecord> requiredModuleRecord = JSHandle<ModuleRecord>::Cast(requiredModule);
            index = SourceTextModule::InnerModuleInstantiation(thread,
                requiredModuleRecord, stack, index, excuteFromJob);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
            // c. Assert: requiredModule.[[Status]] is either "instantiating", "instantiated", or "evaluated".
            ModuleStatus requiredModuleStatus = requiredModule->GetStatus();
            ASSERT((requiredModuleStatus == ModuleStatus::INSTANTIATING ||
                requiredModuleStatus == ModuleStatus::INSTANTIATED || requiredModuleStatus == ModuleStatus::EVALUATED));
            // d. Assert: requiredModule.[[Status]] is "instantiating" if and only if requiredModule is in stack.
            // e. If requiredModule.[[Status]] is "instantiating", then
            if (requiredModuleStatus == ModuleStatus::INSTANTIATING) {
                // d. Assert: requiredModule.[[Status]] is "instantiating" if and only if requiredModule is in stack.
                ASSERT(std::find(stack.begin(), stack.end(), requiredModule) != stack.end());
                // i. Assert: requiredModule is a Source Text Module Record.
                // ii. Set module.[[DFSAncestorIndex]] to min(
                //    module.[[DFSAncestorIndex]], requiredModule.[[DFSAncestorIndex]]).
                int dfsAncIdx = std::min(module->GetDFSAncestorIndex(), requiredModule->GetDFSAncestorIndex());
                module->SetDFSAncestorIndex(dfsAncIdx);
            }
        }
    }
    // Adapter new opcode
    // 10. Perform ? ModuleDeclarationEnvironmentSetup(module).
    if (module->GetIsNewBcVersion()) {
        SourceTextModule::ModuleDeclarationArrayEnvironmentSetup(thread, module);
    } else {
        SourceTextModule::ModuleDeclarationEnvironmentSetup(thread, module);
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
    // 11. Assert: module occurs exactly once in stack.
    // 12. Assert: module.[[DFSAncestorIndex]] is less than or equal to module.[[DFSIndex]].
    int dfsAncIdx = module->GetDFSAncestorIndex();
    int dfsIdx = module->GetDFSIndex();
    ASSERT(dfsAncIdx <= dfsIdx);
    // 13. If module.[[DFSAncestorIndex]] equals module.[[DFSIndex]], then
    if (dfsAncIdx == dfsIdx) {
        // a. Let done be false.
        bool done = false;
        // b. Repeat, while done is false,
        while (!done) {
            // i. Let requiredModule be the last element in stack.
            JSHandle<SourceTextModule> requiredModule = stack.back();
            // ii. Remove the last element of stack.
            stack.pop_back();
            // iii. Set requiredModule.[[Status]] to "instantiated".
            requiredModule->SetStatus(ModuleStatus::INSTANTIATED);
            // iv. If requiredModule and module are the same Module Record, set done to true.
            if (JSTaggedValue::SameValue(module.GetTaggedValue(), requiredModule.GetTaggedValue())) {
                done = true;
            }
        }
    }
    return index;
}

void SourceTextModule::ModuleDeclarationEnvironmentSetup(JSThread *thread,
                                                         const JSHandle<SourceTextModule> &module)
{
    CheckResolvedBinding(thread, module);
    if (module->GetImportEntries().IsUndefined()) {
        return;
    }

    // 2. Assert: All named exports from module are resolvable.
    // 3. Let realm be module.[[Realm]].
    // 4. Assert: realm is not undefined.
    // 5. Let env be NewModuleEnvironment(realm.[[GlobalEnv]]).
    JSHandle<TaggedArray> importEntries(thread, module->GetImportEntries());
    size_t importEntriesLen = importEntries->GetLength();
    JSHandle<NameDictionary> map(NameDictionary::Create(thread,
        NameDictionary::ComputeHashTableSize(importEntriesLen)));
    // 6. Set module.[[Environment]] to env.
    module->SetEnvironment(thread, map);
    // 7. Let envRec be env's EnvironmentRecord.
    JSMutableHandle<JSTaggedValue> envRec(thread, module->GetEnvironment());
    ASSERT(!envRec->IsUndefined());
    // 8. For each ImportEntry Record in in module.[[ImportEntries]], do
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<ImportEntry> in(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> localName(thread, globalConstants->GetUndefined());
    for (size_t idx = 0; idx < importEntriesLen; idx++) {
        in.Update(importEntries->Get(idx));
        localName.Update(in->GetLocalName());
        importName.Update(in->GetImportName());
        moduleRequest.Update(in->GetModuleRequest());
        // a. Let importedModule be ! HostResolveImportedModule(module, in.[[ModuleRequest]]).
        JSMutableHandle<SourceTextModule> importedModule(thread, thread->GlobalConstants()->GetUndefined());
        JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
        if (moduleRecordName.IsUndefined()) {
            JSHandle<JSTaggedValue> importedVal =
            SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            importedModule.Update(JSHandle<SourceTextModule>::Cast(importedVal));
        } else {
            ASSERT(moduleRecordName.IsString());
            JSHandle<JSTaggedValue> importedVal =
                SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            importedModule.Update(JSHandle<SourceTextModule>::Cast(importedVal));
        }
        // c. If in.[[ImportName]] is "*", then
        JSHandle<JSTaggedValue> starString = globalConstants->GetHandledStarString();
        if (JSTaggedValue::SameValue(importName, starString)) {
            // i. Let namespace be ? GetModuleNamespace(importedModule).
            JSHandle<JSTaggedValue> moduleNamespace = SourceTextModule::GetModuleNamespace(thread, importedModule);
            // ii. Perform ! envRec.CreateImmutableBinding(in.[[LocalName]], true).
            // iii. Call envRec.InitializeBinding(in.[[LocalName]], namespace).
            JSHandle<NameDictionary> mapHandle = JSHandle<NameDictionary>::Cast(envRec);
            JSHandle<NameDictionary> newMap = NameDictionary::Put(thread, mapHandle, localName, moduleNamespace,
                                                                  PropertyAttributes::Default());
            envRec.Update(newMap);
        } else {
            // i. Let resolution be ? importedModule.ResolveExport(in.[[ImportName]], « »).
            CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveVector;
            JSHandle<JSTaggedValue> resolution =
                SourceTextModule::ResolveExport(thread, importedModule, importName, resolveVector);
            // ii. If resolution is null or "ambiguous", throw a SyntaxError exception.
            if (resolution->IsNull() || resolution->IsString()) {
                CString msg = "the requested module '" +
                              ConvertToString(moduleRequest.GetTaggedValue()) +
                              "' does not provide an export named '" +
                              ConvertToString(importName.GetTaggedValue());
                if (!module->GetEcmaModuleRecordName().IsUndefined()) {
                    msg += "' which imported by '" + ConvertToString(module->GetEcmaModuleRecordName()) + "'";
                } else {
                    msg += "' which imported by '" + ConvertToString(module->GetEcmaModuleFilename()) + "'";
                }
                THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
            }
            // iii. Call envRec.CreateImportBinding(
            //    in.[[LocalName]], resolution.[[Module]], resolution.[[BindingName]]).
            JSHandle<NameDictionary> mapHandle = JSHandle<NameDictionary>::Cast(envRec);
            JSHandle<NameDictionary> newMap = NameDictionary::Put(thread, mapHandle, localName, resolution,
                                                                  PropertyAttributes::Default());
            envRec.Update(newMap);
        }
    }

    module->SetEnvironment(thread, envRec);
}

void SourceTextModule::ModuleDeclarationArrayEnvironmentSetup(JSThread *thread,
                                                              const JSHandle<SourceTextModule> &module)
{
    CheckResolvedIndexBinding(thread, module);
    if (module->GetImportEntries().IsUndefined()) {
        return;
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // 2. Assert: All named exports from module are resolvable.
    // 3. Let realm be module.[[Realm]].
    // 4. Assert: realm is not undefined.
    // 5. Let env be NewModuleEnvironment(realm.[[GlobalEnv]]).
    JSHandle<TaggedArray> importEntries(thread, module->GetImportEntries());
    size_t importEntriesLen = importEntries->GetLength();
    JSHandle<TaggedArray> arr = factory->NewTaggedArray(importEntriesLen);
    // 6. Set module.[[Environment]] to env.
    module->SetEnvironment(thread, arr);
    // 7. Let envRec be env's EnvironmentRecord.
    JSHandle<TaggedArray> envRec = arr;
    // 8. For each ImportEntry Record in in module.[[ImportEntries]], do
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<ImportEntry> in(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    for (size_t idx = 0; idx < importEntriesLen; idx++) {
        in.Update(importEntries->Get(idx));
        importName.Update(in->GetImportName());
        moduleRequest.Update(in->GetModuleRequest());
        // a. Let importedModule be ! HostResolveImportedModule(module, in.[[ModuleRequest]]).
        JSMutableHandle<SourceTextModule> importedModule(thread, thread->GlobalConstants()->GetUndefined());
        JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
        if (moduleRecordName.IsUndefined()) {
            JSHandle<JSTaggedValue> importedVal =
                SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            importedModule.Update(JSHandle<SourceTextModule>::Cast(importedVal));
        } else {
            ASSERT(moduleRecordName.IsString());
            JSHandle<JSTaggedValue> importedVal =
                SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            importedModule.Update(JSHandle<SourceTextModule>::Cast(importedVal));
        }
        // c. If in.[[ImportName]] is "*", then
        JSHandle<JSTaggedValue> starString = globalConstants->GetHandledStarString();
        if (JSTaggedValue::SameValue(importName, starString)) {
            // need refactor
            return;
        }
        // i. Let resolution be ? importedModule.ResolveExport(in.[[ImportName]], « »).
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveVector;
        JSHandle<JSTaggedValue> resolution =
            SourceTextModule::ResolveExport(thread, importedModule, importName, resolveVector);
        // ii. If resolution is null or "ambiguous", throw a SyntaxError exception.
        if (resolution->IsNull() || resolution->IsString()) {
            if (thread->GetEcmaVM()->EnableReportModuleResolvingFailure()) {
                CString msg = "the requested module '" +
                            ConvertToString(moduleRequest.GetTaggedValue()) +
                            "' does not provide an export named '" +
                            ConvertToString(importName.GetTaggedValue());
                if (!module->GetEcmaModuleRecordName().IsUndefined()) {
                    msg += "' which imported by '" + ConvertToString(module->GetEcmaModuleRecordName()) + "'";
                } else {
                    msg += "' which imported by '" + ConvertToString(module->GetEcmaModuleFilename()) + "'";
                }
                THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
            } else {
                // if in aot compiation, we should skip this error.
                envRec->Set(thread, idx, JSTaggedValue::Hole());
                continue;
            }
        }
        // iii. Call envRec.CreateImportBinding(
        //    in.[[LocalName]], resolution.[[Module]], resolution.[[BindingName]]).
        envRec->Set(thread, idx, resolution);
    }

    module->SetEnvironment(thread, envRec);
}

JSHandle<JSTaggedValue> SourceTextModule::GetModuleNamespace(JSThread *thread,
                                                             const JSHandle<SourceTextModule> &module)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Assert: module is an instance of a concrete subclass of Module Record.
    // 2. Assert: module.[[Status]] is not "uninstantiated".
    ModuleStatus status = module->GetStatus();
    ASSERT(status != ModuleStatus::UNINSTANTIATED);
    // 3. Assert: If module.[[Status]] is "evaluated", module.[[EvaluationError]] is undefined.
    if (status == ModuleStatus::EVALUATED) {
        ASSERT(module->GetEvaluationError() == SourceTextModule::UNDEFINED_INDEX);
    }
    // 4. Let namespace be module.[[Namespace]].
    JSMutableHandle<JSTaggedValue> moduleNamespace(thread, module->GetNamespace().GetWeakRawValue());
    // If namespace is undefined, then
    if (moduleNamespace->IsUndefined()) {
        // a. Let exportedNames be ? module.GetExportedNames(« »).
        JSHandle<TaggedArray> exportStarSet = factory->EmptyArray();
        CVector<std::string> exportedNames = SourceTextModule::GetExportedNames(thread, module, exportStarSet);
        // b. Let unambiguousNames be a new empty List.
        JSHandle<TaggedArray> unambiguousNames = factory->NewTaggedArray(exportedNames.size());
        // c. For each name that is an element of exportedNames, do
        size_t idx = 0;
        for (std::string &name : exportedNames) {
            // i. Let resolution be ? module.ResolveExport(name, « »).
            CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveVector;
            JSHandle<JSTaggedValue> nameHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromStdString(name));
            JSHandle<JSTaggedValue> resolution =
                SourceTextModule::ResolveExport(thread, module, nameHandle, resolveVector);
            // ii. If resolution is a ResolvedBinding Record, append name to unambiguousNames.
            if (resolution->IsResolvedBinding() || resolution->IsResolvedIndexBinding()) {
                unambiguousNames->Set(thread, idx, nameHandle);
                idx++;
            }
        }
        JSHandle<TaggedArray> fixUnambiguousNames = TaggedArray::SetCapacity(thread, unambiguousNames, idx);
        JSHandle<JSTaggedValue> moduleTagged = JSHandle<JSTaggedValue>::Cast(module);
        JSHandle<ModuleNamespace> np =
            ModuleNamespace::ModuleNamespaceCreate(thread, moduleTagged, fixUnambiguousNames);
        moduleNamespace.Update(np.GetTaggedValue());
    }
    return moduleNamespace;
}

int SourceTextModule::Evaluate(JSThread *thread, const JSHandle<SourceTextModule> &module,
                               const void *buffer, size_t size, bool excuteFromJob)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SourceTextModule::Evaluate");
    // 1. Let module be this Source Text Module Record.
    // 2. Assert: module.[[Status]] is "instantiated" or "evaluated".
    [[maybe_unused]] ModuleStatus status = module->GetStatus();
    ASSERT((status == ModuleStatus::INSTANTIATED || status == ModuleStatus::EVALUATED));
    // 3. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    // 4. Let result be InnerModuleEvaluation(module, stack, 0)
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    int result = SourceTextModule::InnerModuleEvaluation(thread, moduleRecord, stack, 0, buffer, size, excuteFromJob);
    // 5. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        // a. For each module m in stack, do
        for (auto mm : stack) {
            // i. Assert: m.[[Status]] is "evaluating".
            ASSERT(mm->GetStatus() == ModuleStatus::EVALUATING);
            // ii. Set m.[[Status]] to "evaluated".
            mm->SetStatus(ModuleStatus::EVALUATED);
            // iii. Set m.[[EvaluationError]] to result.
            mm->SetEvaluationError(result);
        }
        // b. Assert: module.[[Status]] is "evaluated" and module.[[EvaluationError]] is result.
        status = module->GetStatus();
        ASSERT(status == ModuleStatus::EVALUATED && module->GetEvaluationError() == result);
        // c. return result
        return result;
    }
    // 6. Assert: module.[[Status]] is "evaluated" and module.[[EvaluationError]] is undefined.
    status = module->GetStatus();
    ASSERT(status == ModuleStatus::EVALUATED && module->GetEvaluationError() == SourceTextModule::UNDEFINED_INDEX);
    // 7. Assert: stack is empty.
    ASSERT(stack.empty());
    // 8. Return undefined.
    return SourceTextModule::UNDEFINED_INDEX;
}

int SourceTextModule::EvaluateForConcurrent(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    // 1. Let module be this Source Text Module Record.
    // 2. Assert: module.[[Status]] is "instantiated" or "evaluated".
    [[maybe_unused]] ModuleStatus status = module->GetStatus();
    ASSERT((status == ModuleStatus::INSTANTIATED || status == ModuleStatus::EVALUATED));
    // 3. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    // 4. Let result be InnerModuleEvaluation(module, stack, 0)
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    int result = SourceTextModule::ModuleEvaluation(thread, moduleRecord, stack, 0);
    // 5. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        // a. For each module m in stack, do
        for (auto mm : stack) {
            // i. Assert: m.[[Status]] is "evaluating".
            ASSERT(mm->GetStatus() == ModuleStatus::EVALUATING);
            // ii. Set m.[[Status]] to "evaluated".
            mm->SetStatus(ModuleStatus::EVALUATED);
            // iii. Set m.[[EvaluationError]] to result.
            mm->SetEvaluationError(result);
        }
        // b. Assert: module.[[EvaluationError]] is result.
        ASSERT(module->GetEvaluationError() == result);
        // c. return result
        return result;
    }
    // 6. Assert: module.[[EvaluationError]] is undefined.
    ASSERT(module->GetEvaluationError() == SourceTextModule::UNDEFINED_INDEX);
    // 7. Assert: stack is empty.
    ASSERT(stack.empty());
    // 8. Return undefined.
    return SourceTextModule::UNDEFINED_INDEX;
}

int SourceTextModule::InnerModuleEvaluation(JSThread *thread, const JSHandle<ModuleRecord> &moduleRecord,
                                            CVector<JSHandle<SourceTextModule>> &stack, int index,
                                            const void *buffer, size_t size, bool excuteFromJob)
{
    // 1. If module is not a Source Text Module Record, then
    if (!moduleRecord.GetTaggedValue().IsSourceTextModule()) {
        //  a. Perform ? module.Instantiate().
        ModuleRecord::Instantiate(thread, JSHandle<JSTaggedValue>::Cast(moduleRecord));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        //  b. Return index.
        return index;
    }
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    // 2.If module.[[Status]] is "evaluated", then
    ModuleStatus status = module->GetStatus();
    if (status == ModuleStatus::EVALUATED) {
        // a. If module.[[EvaluationError]] is undefined, return index
        if (module->GetEvaluationError() == SourceTextModule::UNDEFINED_INDEX) {
            return index;
        }
        // Otherwise return module.[[EvaluationError]].
        return module->GetEvaluationError();
    }
    // 3. If module.[[Status]] is "evaluating", return index.
    if (status == ModuleStatus::EVALUATING) {
        return index;
    }
    // 4. Assert: module.[[Status]] is "instantiated".
    ASSERT(status == ModuleStatus::INSTANTIATED);
    // 5. Set module.[[Status]] to "evaluating".
    module->SetStatus(ModuleStatus::EVALUATING);
    // 6. Set module.[[DFSIndex]] to index.
    module->SetDFSIndex(index);
    // 7. Set module.[[DFSAncestorIndex]] to index.
    module->SetDFSAncestorIndex(index);
    // 8. Set index to index + 1.
    index++;
    // 9. Append module to stack.
    stack.emplace_back(module);
    // 10. For each String required that is an element of module.[[RequestedModules]], do
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            required.Update(requestedModules->Get(idx));
            // a. Let requiredModule be ! HostResolveImportedModule(module, required).
            JSMutableHandle<SourceTextModule> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
            JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
            if (moduleRecordName.IsUndefined()) {
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModule(thread, module, required);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, SourceTextModule::UNDEFINED_INDEX);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
            } else {
                ASSERT(moduleRecordName.IsString());
                JSHandle<JSTaggedValue> requiredVal =
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, required);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, SourceTextModule::UNDEFINED_INDEX);
                requiredModule.Update(JSHandle<SourceTextModule>::Cast(requiredVal));
            }
            ModuleTypes moduleType = requiredModule->GetTypes();
            if (SourceTextModule::IsNativeModule(moduleType)) {
                InstantiateNativeModule(thread, module, requiredModule, required, moduleType);
                requiredModule->SetStatus(ModuleStatus::EVALUATED);
                continue;
            }
            // if requiredModule is jsonModule, then don't need to execute.
            if (moduleType == ModuleTypes::JSON_MODULE) {
                requiredModule->SetStatus(ModuleStatus::EVALUATED);
                continue;
            }
            // c. Set index to ? InnerModuleEvaluation(requiredModule, stack, index).
            JSHandle<ModuleRecord> requiredModuleRecord = JSHandle<ModuleRecord>::Cast(requiredModule);
            index = SourceTextModule::InnerModuleEvaluation(thread, requiredModuleRecord, stack, index);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
            // d. Assert: requiredModule.[[Status]] is either "evaluating" or "evaluated".
            ModuleStatus requiredModuleStatus = requiredModule->GetStatus();
            ASSERT((requiredModuleStatus == ModuleStatus::EVALUATING ||
                    requiredModuleStatus == ModuleStatus::EVALUATED));
            // e. Assert: requiredModule.[[Status]] is "evaluating" if and only if requiredModule is in stack.
            if (requiredModuleStatus == ModuleStatus::EVALUATING) {
                ASSERT(std::find(stack.begin(), stack.end(), requiredModule) != stack.end());
            }
            // f. If requiredModule.[[Status]] is "evaluating", then
            if (requiredModuleStatus == ModuleStatus::EVALUATING) {
                // i. Assert: requiredModule is a Source Text Module Record.
                // ii. Set module.[[DFSAncestorIndex]] to min(
                //    module.[[DFSAncestorIndex]], requiredModule.[[DFSAncestorIndex]]).
                int dfsAncIdx = std::min(module->GetDFSAncestorIndex(), requiredModule->GetDFSAncestorIndex());
                module->SetDFSAncestorIndex(dfsAncIdx);
            }
            // if requiredModule is CommonJS Module, instantiate here (after CommonJS execution).
            if (moduleType == ModuleTypes::CJS_MODULE) {
                InstantiateCJS(thread, module, requiredModule);
            }
        }
    }

    // 11. Perform ? ModuleExecution(module).
    SourceTextModule::ModuleExecution(thread, module, buffer, size, excuteFromJob);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
    // 12. Assert: module occurs exactly once in stack.
    // 13. Assert: module.[[DFSAncestorIndex]] is less than or equal to module.[[DFSIndex]].
    int dfsAncIdx = module->GetDFSAncestorIndex();
    int dfsIdx = module->GetDFSIndex();
    ASSERT(dfsAncIdx <= dfsIdx);
    // 14. If module.[[DFSAncestorIndex]] equals module.[[DFSIndex]], then
    if (dfsAncIdx == dfsIdx) {
        // a. Let done be false.
        bool done = false;
        // b. Repeat, while done is false,
        while (!done) {
            // i. Let requiredModule be the last element in stack.
            JSHandle<SourceTextModule> requiredModule = stack.back();
            // ii. Remove the last element of stack.
            stack.pop_back();
            // iii. Set requiredModule.[[Status]] to "evaluated".
            requiredModule->SetStatus(ModuleStatus::EVALUATED);
            // iv. If requiredModule and module are the same Module Record, set done to true.
            if (JSTaggedValue::SameValue(module.GetTaggedValue(), requiredModule.GetTaggedValue())) {
                done = true;
            }
        }
    }
    return index;
}

int SourceTextModule::ModuleEvaluation(JSThread *thread, const JSHandle<ModuleRecord> &moduleRecord,
                                       CVector<JSHandle<SourceTextModule>> &stack, int index)
{
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            required.Update(requestedModules->Get(idx));
            JSMutableHandle<SourceTextModule> requiredModule(thread, thread->GlobalConstants()->GetUndefined());
            JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
            if (moduleRecordName.IsUndefined()) {
                requiredModule.Update(SourceTextModule::HostResolveImportedModule(thread, module, required));
            } else {
                ASSERT(moduleRecordName.IsString());
                requiredModule.Update(SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, required));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
            }
            ModuleTypes moduleType = requiredModule->GetTypes();
            if (SourceTextModule::IsNativeModule(moduleType)) {
                InstantiateNativeModule(thread, module, requiredModule, required, moduleType);
                requiredModule->SetStatus(ModuleStatus::EVALUATED);
                continue;
            }
            if (moduleType == ModuleTypes::JSON_MODULE) {
                requiredModule->SetStatus(ModuleStatus::EVALUATED);
                continue;
            }
            JSHandle<ModuleRecord> requiredModuleRecord = JSHandle<ModuleRecord>::Cast(requiredModule);
            index = SourceTextModule::InnerModuleEvaluation(thread, requiredModuleRecord, stack, index);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
            [[maybe_unused]] ModuleStatus requiredModuleStatus = requiredModule->GetStatus();
            ASSERT(requiredModuleStatus == ModuleStatus::EVALUATED);
            if (moduleType == ModuleTypes::CJS_MODULE) {
                InstantiateCJS(thread, module, requiredModule);
            }
        }
    }
    return index;
}

void SourceTextModule::ModuleExecution(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                       const void *buffer, size_t size, bool excuteFromJob)
{
    CString moduleFilenameStr = ConvertToString(module->GetEcmaModuleFilename());
    CString entryPoint = GetRecordName(module.GetTaggedValue());

    std::shared_ptr<JSPandaFile> jsPandaFile;
    if (buffer != nullptr) {
        jsPandaFile =
            JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFilenameStr, entryPoint, buffer, size);
    } else {
        jsPandaFile =
            JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFilenameStr, entryPoint);
    }

    if (jsPandaFile == nullptr) {
        CString msg = "Load file with filename '" + moduleFilenameStr + "' failed, recordName '" +
                      entryPoint + "'";
        THROW_ERROR(thread, ErrorType::REFERENCE_ERROR, msg.c_str());
    }
    JSPandaFileExecutor::Execute(thread, jsPandaFile.get(), entryPoint, excuteFromJob);
}

void SourceTextModule::AddImportEntry(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                      const JSHandle<ImportEntry> &importEntry, size_t idx, uint32_t len)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue importEntries = module->GetImportEntries();
    if (importEntries.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(len);
        array->Set(thread, idx, importEntry.GetTaggedValue());
        module->SetImportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, importEntries);
        if (len > entries->GetLength()) {
            entries = TaggedArray::SetCapacity(thread, entries, len);
            entries->Set(thread, idx, importEntry.GetTaggedValue());
            module->SetImportEntries(thread, entries);
            return;
        }
        entries->Set(thread, idx, importEntry.GetTaggedValue());
    }
}

void SourceTextModule::AddLocalExportEntry(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                           const JSHandle<LocalExportEntry> &exportEntry, size_t idx, uint32_t len)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue localExportEntries = module->GetLocalExportEntries();
    if (localExportEntries.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(len);
        array->Set(thread, idx, exportEntry.GetTaggedValue());
        module->SetLocalExportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, localExportEntries);
        entries->Set(thread, idx, exportEntry.GetTaggedValue());
    }
}

void SourceTextModule::AddIndirectExportEntry(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                              const JSHandle<IndirectExportEntry> &exportEntry,
                                              size_t idx, uint32_t len)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue indirectExportEntries = module->GetIndirectExportEntries();
    if (indirectExportEntries.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(len);
        array->Set(thread, idx, exportEntry.GetTaggedValue());
        module->SetIndirectExportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, indirectExportEntries);
        entries->Set(thread, idx, exportEntry.GetTaggedValue());
    }
}

void SourceTextModule::AddStarExportEntry(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                          const JSHandle<StarExportEntry> &exportEntry, size_t idx, uint32_t len)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue starExportEntries = module->GetStarExportEntries();
    if (starExportEntries.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(len);
        array->Set(thread, idx, exportEntry.GetTaggedValue());
        module->SetStarExportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, starExportEntries);
        entries->Set(thread, idx, exportEntry.GetTaggedValue());
    }
}

JSTaggedValue SourceTextModule::GetModuleValue(JSThread *thread, int32_t index, bool isThrow)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue dictionary = GetNameDictionary();
    if (dictionary.IsUndefined()) {
        if (isThrow) {
            THROW_REFERENCE_ERROR_AND_RETURN(thread, "module environment is undefined", JSTaggedValue::Exception());
        }
        return JSTaggedValue::Hole();
    }

    TaggedArray *array = TaggedArray::Cast(dictionary.GetTaggedObject());
    return array->Get(index);
}

JSTaggedValue SourceTextModule::GetModuleValue(JSThread *thread, JSTaggedValue key, bool isThrow)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue dictionary = GetNameDictionary();
    if (dictionary.IsUndefined()) {
        if (isThrow) {
            THROW_REFERENCE_ERROR_AND_RETURN(thread, "module environment is undefined", JSTaggedValue::Exception());
        }
        return JSTaggedValue::Hole();
    }

    NameDictionary *dict = NameDictionary::Cast(dictionary.GetTaggedObject());
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        return dict->GetValue(entry);
    }

    // when key is exportName, need to get localName
    JSTaggedValue exportEntriesTv = GetLocalExportEntries();
    if (!exportEntriesTv.IsUndefined()) {
        JSTaggedValue resolution = FindByExport(exportEntriesTv, key, dictionary);
        if (!resolution.IsHole()) {
            return resolution;
        }
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue SourceTextModule::FindByExport(const JSTaggedValue &exportEntriesTv, const JSTaggedValue &key,
                                             const JSTaggedValue &dictionary)
{
    DISALLOW_GARBAGE_COLLECTION;
    NameDictionary *dict = NameDictionary::Cast(dictionary.GetTaggedObject());
    TaggedArray *exportEntries = TaggedArray::Cast(exportEntriesTv.GetTaggedObject());
    size_t exportEntriesLen = exportEntries->GetLength();
    for (size_t idx = 0; idx < exportEntriesLen; idx++) {
        LocalExportEntry *ee = LocalExportEntry::Cast(exportEntries->Get(idx).GetTaggedObject());
        if (!JSTaggedValue::SameValue(ee->GetExportName(), key)) {
            continue;
        }
        JSTaggedValue localName = ee->GetLocalName();
        int entry = dict->FindEntry(localName);
        if (entry != -1) {
            return dict->GetValue(entry);
        }
    }

    return JSTaggedValue::Hole();
}

void SourceTextModule::StoreModuleValue(JSThread *thread, int32_t index, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<SourceTextModule> module(thread, this);
    JSTaggedValue localExportEntries = module->GetLocalExportEntries();
    ASSERT(localExportEntries.IsTaggedArray());

    JSHandle<JSTaggedValue> data(thread, module->GetNameDictionary());
    if (data->IsUndefined()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        uint32_t size = TaggedArray::Cast(localExportEntries.GetTaggedObject())->GetLength();
        ASSERT(index < static_cast<int32_t>(size));
        data = JSHandle<JSTaggedValue>(factory->NewTaggedArray(size));
        module->SetNameDictionary(thread, data);
    }
    JSHandle<TaggedArray> arr(data);
    arr->Set(thread, index, value);
}

void SourceTextModule::StoreModuleValue(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                        const JSHandle<JSTaggedValue> &value)
{
    JSHandle<SourceTextModule> module(thread, this);
    JSMutableHandle<JSTaggedValue> data(thread, module->GetNameDictionary());
    if (data->IsUndefined()) {
        data.Update(NameDictionary::Create(thread, DEFAULT_DICTIONART_CAPACITY));
    }
    JSHandle<NameDictionary> dataDict = JSHandle<NameDictionary>::Cast(data);
    data.Update(NameDictionary::Put(thread, dataDict, key, value, PropertyAttributes::Default()));

    module->SetNameDictionary(thread, data);
}

void SourceTextModule::SetExportName(JSThread *thread, const JSHandle<JSTaggedValue> &moduleRequest,
                                     const JSHandle<SourceTextModule> &module,
                                     CVector<std::string> &exportedNames, JSHandle<TaggedArray> &newExportStarSet)

{
    JSMutableHandle<SourceTextModule> requestedModule(thread, thread->GlobalConstants()->GetUndefined());
    JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
    if (moduleRecordName.IsUndefined()) {
        JSHandle<JSTaggedValue> requestedVal =
            SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        requestedModule.Update(JSHandle<SourceTextModule>::Cast(requestedVal));
    } else {
        ASSERT(moduleRecordName.IsString());
        JSHandle<JSTaggedValue> requestedVal =
            SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        requestedModule.Update(JSHandle<SourceTextModule>::Cast(requestedVal));
    }
    // b. Let starNames be ? requestedModule.GetExportedNames(exportStarSet).
    CVector<std::string> starNames =
        SourceTextModule::GetExportedNames(thread, requestedModule, newExportStarSet);
    // c. For each element n of starNames, do
    for (std::string &nn : starNames) {
        // i. If SameValue(n, "default") is false, then
        if (nn != "default" && std::find(exportedNames.begin(), exportedNames.end(), nn) == exportedNames.end()) {
            // 1. If n is not an element of exportedNames, then
            //    a. Append n to exportedNames.
            exportedNames.emplace_back(nn);
        }
    }
}

JSHandle<JSTaggedValue> SourceTextModule::GetStarResolution(JSThread *thread,
                                                            const JSHandle<JSTaggedValue> &exportName,
                                                            const JSHandle<JSTaggedValue> &moduleRequest,
                                                            const JSHandle<SourceTextModule> &module,
                                                            JSMutableHandle<JSTaggedValue> &starResolution,
                                                            CVector<std::pair<JSHandle<SourceTextModule>,
                                                            JSHandle<JSTaggedValue>>> &resolveVector)
{
    auto globalConstants = thread->GlobalConstants();
    // a. Let importedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
    JSMutableHandle<SourceTextModule> importedModule(thread, thread->GlobalConstants()->GetUndefined());
    JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
    if (moduleRecordName.IsUndefined()) {
        JSHandle<JSTaggedValue> importedVal =
            SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        importedModule.Update(JSHandle<SourceTextModule>::Cast(importedVal));
    } else {
        ASSERT(moduleRecordName.IsString());
        JSHandle<JSTaggedValue> importedVal =
            SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        importedModule.Update(JSHandle<SourceTextModule>::Cast(importedVal));
    }
    // b. Let resolution be ? importedModule.ResolveExport(exportName, resolveVector).
    JSHandle<JSTaggedValue> resolution =
        SourceTextModule::ResolveExport(thread, importedModule, exportName, resolveVector);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // if step into GetStarResolution in aot phase, the module must be a normal SourceTextModule not an empty
    // aot module. Sometimes for normal module, if indirectExportEntries, localExportEntries, starExportEntries
    // all don't have right exportName which means the export element is not from this module,
    // it should return null but now will be hole.
    if (!thread->GetEcmaVM()->EnableReportModuleResolvingFailure() && resolution->IsHole()) {
        return globalConstants->GetHandledNull();
    }
    // c. If resolution is "ambiguous", return "ambiguous".
    if (resolution->IsString()) { // if resolution is string, resolution must be "ambiguous"
        return globalConstants->GetHandledAmbiguousString();
    }
    // d. If resolution is not null, then
    if (resolution->IsNull()) {
        return globalConstants->GetHandledNull();
    }
    // i. Assert: resolution is a ResolvedBinding Record.
    ASSERT(resolution->IsResolvedBinding() || resolution->IsResolvedIndexBinding());
    // ii. If starResolution is null, set starResolution to resolution.
    if (starResolution->IsNull()) {
        starResolution.Update(resolution.GetTaggedValue());
    } else {
        // 1. Assert: There is more than one * import that includes the requested name.
        // 2. If resolution.[[Module]] and starResolution.[[Module]] are not the same Module Record or
        // SameValue(
        //    resolution.[[BindingName]], starResolution.[[BindingName]]) is false, return "ambiguous".
        // Adapter new opcode
        if (resolution->IsResolvedBinding()) {
            JSHandle<ResolvedBinding> resolutionBd = JSHandle<ResolvedBinding>::Cast(resolution);
            JSHandle<ResolvedBinding> starResolutionBd = JSHandle<ResolvedBinding>::Cast(starResolution);
            if ((!JSTaggedValue::SameValue(resolutionBd->GetModule(), starResolutionBd->GetModule())) ||
                (!JSTaggedValue::SameValue(
                    resolutionBd->GetBindingName(), starResolutionBd->GetBindingName()))) {
                return globalConstants->GetHandledAmbiguousString();
            }
        } else {
            JSHandle<ResolvedIndexBinding> resolutionBd = JSHandle<ResolvedIndexBinding>::Cast(resolution);
            JSHandle<ResolvedIndexBinding> starResolutionBd = JSHandle<ResolvedIndexBinding>::Cast(starResolution);
            if ((!JSTaggedValue::SameValue(resolutionBd->GetModule(), starResolutionBd->GetModule())) ||
                resolutionBd->GetIndex() != starResolutionBd->GetIndex()) {
                return globalConstants->GetHandledAmbiguousString();
            }
        }
    }
    return resolution;
}

template <typename T>
void SourceTextModule::AddExportName(JSThread *thread, const JSTaggedValue &exportEntry,
                                     CVector<std::string> &exportedNames)
{
    if (!exportEntry.IsUndefined()) {
        JSMutableHandle<T> ee(thread, thread->GlobalConstants()->GetUndefined());
        JSHandle<TaggedArray> exportEntries(thread, exportEntry);
        size_t exportEntriesLen = exportEntries->GetLength();
        for (size_t idx = 0; idx < exportEntriesLen; idx++) {
            ee.Update(exportEntries->Get(idx));
            // a. Assert: module provides the direct binding for this export.
            // b. Append e.[[ExportName]] to exportedNames.
            std::string exportName = EcmaStringAccessor(ee->GetExportName()).ToStdString();
            exportedNames.emplace_back(exportName);
        }
    }
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveElementOfObject(JSThread *thread,
                                                                 const JSHandle<JSHClass> &hclass,
                                                                 const JSHandle<JSTaggedValue> &exportName,
                                                                 const JSHandle<SourceTextModule> &module)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int idx = JSHClass::FindPropertyEntry(thread, *hclass, exportName.GetTaggedValue());
    if (idx != -1) {
        return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedIndexBindingRecord(module, idx));
    }
    return thread->GlobalConstants()->GetHandledUndefined();
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveLocalExport(JSThread *thread,
                                                             const JSHandle<JSTaggedValue> &exportEntry,
                                                             const JSHandle<JSTaggedValue> &exportName,
                                                             const JSHandle<SourceTextModule> &module)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> localName(thread, thread->GlobalConstants()->GetUndefined());

    JSHandle<TaggedArray> localExportEntries(exportEntry);
    size_t localExportEntriesLen = localExportEntries->GetLength();
    for (size_t idx = 0; idx < localExportEntriesLen; idx++) {
        ee.Update(localExportEntries->Get(idx));
        // a. If SameValue(exportName, e.[[ExportName]]) is true, then
        // if module is type of CommonJS or native, export first, check after execution.
        auto moduleType = module->GetTypes();
        if (moduleType == ModuleTypes::CJS_MODULE) {
            return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedBindingRecord(module, exportName));
        }

        if ((JSTaggedValue::SameValue(ee->GetExportName(), exportName.GetTaggedValue())) ||
                IsNativeModule(moduleType)) {
            // Adapter new module
            if (module->GetIsNewBcVersion()) {
                return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedIndexBindingRecord(module,
                    ee->GetLocalIndex()));
            }
            // i. Assert: module provides the direct binding for this export.
            // ii. Return ResolvedBinding Record { [[Module]]: module, [[BindingName]]: e.[[LocalName]] }.
            localName.Update(ee->GetLocalName());
            return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedBindingRecord(module, localName));
        }
    }
    return thread->GlobalConstants()->GetHandledUndefined();
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveIndirectExport(JSThread *thread,
                                                                const JSHandle<JSTaggedValue> &exportEntry,
                                                                const JSHandle<JSTaggedValue> &exportName,
                                                                const JSHandle<SourceTextModule> &module,
                                                                CVector<std::pair<JSHandle<SourceTextModule>,
                                                                JSHandle<JSTaggedValue>>> &resolveVector)
{
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<IndirectExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    JSHandle<TaggedArray> indirectExportEntries(exportEntry);
    size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
    for (size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
        ee.Update(indirectExportEntries->Get(idx));
        //  a. If SameValue(exportName, e.[[ExportName]]) is true, then
        if (JSTaggedValue::SameValue(exportName.GetTaggedValue(), ee->GetExportName())) {
            // i. Assert: module imports a specific binding for this export.
            // ii. Let importedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
            moduleRequest.Update(ee->GetModuleRequest());
            JSMutableHandle<SourceTextModule> requestedModule(thread, thread->GlobalConstants()->GetUndefined());
            JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
            if (moduleRecordName.IsUndefined()) {
                requestedModule.Update(SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest));
            } else {
                ASSERT(moduleRecordName.IsString());
                requestedModule.Update(
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest));
            }
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            // iii. Return importedModule.ResolveExport(e.[[ImportName]], resolveVector).
            importName.Update(ee->GetImportName());
            return SourceTextModule::ResolveExport(thread, requestedModule, importName, resolveVector);
        }
    }
    return thread->GlobalConstants()->GetHandledUndefined();
}

void SourceTextModule::CheckResolvedBinding(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    auto globalConstants = thread->GlobalConstants();
    // 1. For each ExportEntry Record e in module.[[IndirectExportEntries]], do
    JSTaggedValue indirectExportEntriesTv = module->GetIndirectExportEntries();
    if (indirectExportEntriesTv.IsUndefined()) {
        return;
    }

    JSMutableHandle<IndirectExportEntry> ee(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> exportName(thread, globalConstants->GetUndefined());
    JSHandle<TaggedArray> indirectExportEntries(thread, indirectExportEntriesTv);
    size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
    for (size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
        ee.Update(indirectExportEntries->Get(idx));
        // a. Let resolution be ? module.ResolveExport(e.[[ExportName]], « »).
        exportName.Update(ee->GetExportName());
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveVector;
        JSHandle<JSTaggedValue> resolution =
            SourceTextModule::ResolveExport(thread, module, exportName, resolveVector);
        // b. If resolution is null or "ambiguous", throw a SyntaxError exception.
        if (resolution->IsNull() || resolution->IsString()) {
            CString msg = "the requested module '" +
                          ConvertToString(ee->GetModuleRequest()) +
                          "' does not provide an export named '" +
                          ConvertToString(exportName.GetTaggedValue());
            if (!module->GetEcmaModuleRecordName().IsUndefined()) {
                msg += "' which exported by '" + ConvertToString(module->GetEcmaModuleRecordName()) + "'";
            } else {
                msg += "' which exported by '" + ConvertToString(module->GetEcmaModuleFilename()) + "'";
            }
            THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
        }
        // c. Assert: resolution is a ResolvedBinding Record.
        ASSERT(resolution->IsResolvedBinding());
    }
}

void SourceTextModule::CheckResolvedIndexBinding(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    auto globalConstants = thread->GlobalConstants();
    // 1. For each ExportEntry Record e in module.[[IndirectExportEntries]], do
    JSTaggedValue indirectExportEntriesTv = module->GetIndirectExportEntries();
    if (indirectExportEntriesTv.IsUndefined()) {
        return;
    }

    JSMutableHandle<IndirectExportEntry> ee(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> exportName(thread, globalConstants->GetUndefined());
    JSHandle<TaggedArray> indirectExportEntries(thread, indirectExportEntriesTv);
    size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
    for (size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
        ee.Update(indirectExportEntries->Get(idx));
        // a. Let resolution be ? module.ResolveExport(e.[[ExportName]], « »).
        exportName.Update(ee->GetExportName());
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveVector;
        JSHandle<JSTaggedValue> resolution =
            SourceTextModule::ResolveExport(thread, module, exportName, resolveVector);
        // b. If resolution is null or "ambiguous", throw a SyntaxError exception.
        if (resolution->IsNull() || resolution->IsString()) {
            CString msg = "the requested module '" +
                          ConvertToString(ee->GetModuleRequest()) +
                          "' does not provide an export named '" +
                          ConvertToString(exportName.GetTaggedValue());
            if (!module->GetEcmaModuleRecordName().IsUndefined()) {
                msg += "' which exported by '" + ConvertToString(module->GetEcmaModuleRecordName()) + "'";
            } else {
                msg += "' which exported by '" + ConvertToString(module->GetEcmaModuleFilename()) + "'";
            }
            THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
        }
    }
}

JSTaggedValue SourceTextModule::GetModuleName(JSTaggedValue currentModule)
{
    SourceTextModule *module = SourceTextModule::Cast(currentModule.GetTaggedObject());
    JSTaggedValue recordName = module->GetEcmaModuleRecordName();
    if (recordName.IsUndefined()) {
        return module->GetEcmaModuleFilename();
    }
    return recordName;
}

bool SourceTextModule::IsDynamicModule(LoadingTypes types)
{
    return types == LoadingTypes::DYNAMITC_MODULE;
}

CString SourceTextModule::GetRecordName(JSTaggedValue module)
{
    if (module.IsString()) {
        return ConvertToString(module);
    }
    if (module.IsSourceTextModule()) {
        SourceTextModule *sourceTextModule = SourceTextModule::Cast(module.GetTaggedObject());
        if (sourceTextModule->GetEcmaModuleRecordName().IsString()) {
            return ConvertToString(sourceTextModule->GetEcmaModuleRecordName());
        }
    }
    return "";
}

int SourceTextModule::GetExportObjectIndex(EcmaVM *vm, JSHandle<SourceTextModule> ecmaModule,
                                           const std::string &key)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<TaggedArray> localExportEntries(thread, ecmaModule->GetLocalExportEntries());
    size_t exportEntriesLen = localExportEntries->GetLength();
    // 0: There's only one export value "default"
    if (exportEntriesLen == 0) {
        return exportEntriesLen;
    }
    JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    ASSERT(exportEntriesLen > 1); // 1:  The number of export objects exceeds 1
    for (size_t idx = 0; idx < exportEntriesLen; idx++) {
        ee.Update(localExportEntries->Get(idx));
        if (EcmaStringAccessor(ee->GetExportName()).ToStdString() == key) {
            ASSERT(idx <= static_cast<size_t>(INT_MAX));
            return static_cast<int>(idx);
        }
    }
    return -1;
}
} // namespace panda::ecmascript