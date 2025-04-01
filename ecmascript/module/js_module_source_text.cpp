/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/module/module_logger.h"
#include "ecmascript/module/module_message_helper.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/module_tools.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/module/module_resolver.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/runtime_lock.h"
#include "ecmascript/patch/quick_fix_manager.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;
using StringHelper = base::StringHelper;
using GlobalError = containers::ContainerError;

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
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        // 7. For each ExportEntry Record e in module.[[StarExportEntries]], do
        JSHandle<TaggedArray> starExportEntries(thread, entryValue);
        size_t starExportEntriesLen = starExportEntries->GetLength();
        for (size_t idx = 0; idx < starExportEntriesLen; idx++) {
            ee.Update(starExportEntries->Get(idx));
            // a. Let requestedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
            JSHandle<SourceTextModule> requestedModule =
                GetRequestedModule(thread, requestedModules, ee->GetModuleRequestIndex());
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exportedNames);
            SetExportName(thread, requestedModule, exportedNames, newExportStarSet);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exportedNames);
        }
    }
    return exportedNames;
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
    if (exports->IsNativeModuleFailureInfo()) {
        if (!EcmaVM::GetErrorInfoEnhance()) {
            return globalConstants->GetHandledNull();
        }
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

JSHandle<JSTaggedValue> SourceTextModule::ResolveNativeStarExport(JSThread *thread,
                                                                  const JSHandle<SourceTextModule> &nativeModule,
                                                                  const JSHandle<JSTaggedValue> &exportName)
{
    ModuleTypes moduleType = nativeModule->GetTypes();
    if (!SourceTextModule::EvaluateNativeModule(thread, nativeModule, moduleType)) {
        return thread->GlobalConstants()->GetHandledNull();
    }
    JSHandle<JSTaggedValue> nativeExports(thread, nativeModule->GetModuleValue(thread, 0, false));
    return SourceTextModule::ResolveExportObject(thread, nativeModule, nativeExports, exportName);
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveCjsStarExport(JSThread *thread,
                                                               const JSHandle<SourceTextModule> &cjsModule,
                                                               const JSHandle<JSTaggedValue> &exportName)
{
    if (cjsModule->GetStatus() < ModuleStatus::EVALUATED) {
        SourceTextModule::ModuleExecution(thread, cjsModule);
        SourceTextModule::RecordEvaluatedOrError(thread, cjsModule);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, thread->GlobalConstants()->GetHandledNull());
    }

    CString moduleName = GetModuleName(cjsModule.GetTaggedValue());
    JSHandle<JSTaggedValue> cjsModuleName(thread->GetEcmaVM()->GetFactory()->NewFromUtf8(moduleName));
    JSHandle<JSTaggedValue> cjsExports = CjsModule::SearchFromModuleCache(thread, cjsModuleName);
    return SourceTextModule::ResolveExportObject(thread, cjsModule, cjsExports, exportName);
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
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        if (!resolution->IsUndefined()) {
            return resolution;
        }
    }
    // 6. If SameValue(exportName, "default") is true, then
    JSHandle<JSTaggedValue> defaultString = globalConstants->GetHandledDefaultString();
    if (JSTaggedValue::SameValue(exportName, defaultString)) {
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
        return starResolution;
    }
    JSMutableHandle<StarExportEntry> ee(thread, globalConstants->GetUndefined());
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
    JSHandle<TaggedArray> starExportEntries(thread, starExportEntriesTv);
    size_t starExportEntriesLen = starExportEntries->GetLength();
    for (size_t idx = 0; idx < starExportEntriesLen; idx++) {
        ee.Update(starExportEntries->Get(idx));
        // a. Let importedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
        JSHandle<SourceTextModule> requestedModule =
            GetRequestedModule(thread, requestedModules, ee->GetModuleRequestIndex());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        JSHandle<JSTaggedValue> result = GetStarResolution(thread, exportName, requestedModule,
                                                           starResolution, resolveVector);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        if (result->IsString() || result->IsException()) {
            return result;
        }
    }
    // 9. Return starResolution.
    return starResolution;
}

bool SourceTextModule::IsNativeModule(const CString &moduleRequestName)
{
    if (moduleRequestName[0] != '@' ||
        StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::PREFIX_BUNDLE) ||
        StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::PREFIX_PACKAGE) ||
        StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::PREFIX_NORMALIZED_NOT_SO) ||
        moduleRequestName.find(':') == CString::npos) {
        return false;
    }
    return true;
}

ModuleTypes SourceTextModule::GetNativeModuleType(const CString &moduleRequestName)
{
    if (StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::REQUIRE_NAPI_OHOS_PREFIX)) {
        return ModuleTypes::OHOS_MODULE;
    }
    /*
    * moduleRequestName: @app:xxx/xxx
    *                  : @normalized:Y&xxx
    */
    if (StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::REQUIRE_NAPI_APP_PREFIX) ||
        StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::PREFIX_NORMALIZED_SO)) {
        return ModuleTypes::APP_MODULE;
    }
    if (StringHelper::StringStartWith(moduleRequestName, ModulePathHelper::REQUIRE_NAITVE_MODULE_PREFIX)) {
        return ModuleTypes::NATIVE_MODULE;
    }
    return ModuleTypes::INTERNAL_MODULE;
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

void SourceTextModule::MakeNormalizedAppArgs(const EcmaVM *vm, std::vector<Local<JSValueRef>> &arguments,
    const CString &soPath, const CString &moduleName)
{
    CString soName = ModulePathHelper::GetNormalizedPathFromOhmUrl(soPath);
    CString path = base::ConcatToCString(ModulePathHelper::GetBundleNameFromNormalized(vm, soPath),
                                         PathHelper::SLASH_TAG, moduleName);
    // use module name as so name
    arguments[0] = StringRef::NewFromUtf8(vm, soName.c_str());
    arguments.emplace_back(BooleanRef::New(vm, true));
    arguments.emplace_back(StringRef::NewFromUtf8(vm, path.c_str()));
}

void SourceTextModule::MakeAppArgs(const EcmaVM *vm, std::vector<Local<JSValueRef>> &arguments,
    const CString &soPath, const CString &moduleName, const CString &requestName)
{
    if (!StringHelper::StringStartWith(requestName, ModulePathHelper::REQUIRE_NAPI_APP_PREFIX)) {
        return MakeNormalizedAppArgs(vm, arguments, soPath, moduleName);
    }
    size_t pos = soPath.find_last_of(PathHelper::SLASH_TAG);
    if (pos == CString::npos) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Invalid native module " << soPath;
        UNREACHABLE();
    }
    CString soName = soPath.substr(pos + 1);
    CString path = soPath.substr(0, pos);
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

Local<JSValueRef> SourceTextModule::LoadNativeModuleImpl(EcmaVM *vm, JSThread *thread,
    const JSHandle<SourceTextModule> &requiredModule, ModuleTypes moduleType)
{
    CString moduleRequestName = requiredModule->GetEcmaModuleRecordNameString();
    ModuleTraceScope moduleTraceScope(thread, "SourceTextModule::LoadNativeModule:" + moduleRequestName);
    ModuleLogger *moduleLogger = thread->GetModuleLogger();
    if (moduleLogger != nullptr) {
        moduleLogger->SetStartTime(moduleRequestName);
    }
    CString soName = PathHelper::GetStrippedModuleName(moduleRequestName);

    CString fileName = requiredModule->GetEcmaModuleFilenameString();
    CString moduleName = ModulePathHelper::GetModuleNameWithBaseFile(fileName);
    std::vector<Local<JSValueRef>> arguments;
    LOG_FULL(DEBUG) << "Request module is " << moduleRequestName;

    arguments.emplace_back(StringRef::NewFromUtf8(vm, soName.c_str()));
    if (moduleType == ModuleTypes::APP_MODULE) {
        MakeAppArgs(vm, arguments, soName, moduleName, moduleRequestName);
    } else if (moduleType == ModuleTypes::INTERNAL_MODULE) {
        MakeInternalArgs(vm, arguments, moduleRequestName);
    }
    auto maybeFuncRef = GetRequireNativeModuleFunc(vm, moduleType);
    // some function(s) may not registered in global object for non-main thread
    if (!maybeFuncRef->IsFunction(vm)) {
        LOG_FULL(WARN) << "Not found require func";
        if (moduleLogger != nullptr) {
            moduleLogger->SetEndTime(moduleRequestName);
        }
        return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    }

    Local<FunctionRef> funcRef = maybeFuncRef;
    auto exportObject = funcRef->Call(vm, JSValueRef::Undefined(vm), arguments.data(), arguments.size());
    if (moduleLogger != nullptr) {
        moduleLogger->SetEndTime(moduleRequestName);
    }
    return exportObject;
}

Local<JSValueRef> SourceTextModule::LoadNativeModuleMayThrowError(JSThread *thread,
    const JSHandle<SourceTextModule> &requiredModule, ModuleTypes moduleType)
{
    EcmaVM *vm = thread->GetEcmaVM();
    EscapeLocalScope scope(vm);

    auto exportObject = LoadNativeModuleImpl(vm, thread, requiredModule, moduleType);
    if (exportObject->IsNativeModuleFailureInfoObject(vm) || exportObject->IsUndefined()) {
        CString errorMsg = "load native module failed.";
        LOG_FULL(ERROR) << errorMsg.c_str();
        auto error = GlobalError::ReferenceError(thread, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error,
            JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined())));
    }
    return scope.Escape(exportObject);
}

bool SourceTextModule::LoadNativeModule(JSThread *thread, const JSHandle<SourceTextModule> &requiredModule,
                                        ModuleTypes moduleType)
{
    EcmaVM *vm = thread->GetEcmaVM();
    [[maybe_unused]] LocalScope scope(vm);
    
    auto exportObject = LoadNativeModuleImpl(vm, thread, requiredModule, moduleType);
    if (UNLIKELY(exportObject->IsNativeModuleFailureInfoObject(vm))) {
        if (EcmaVM::GetErrorInfoEnhance()) {
            SourceTextModule::StoreModuleValue(thread, requiredModule, 0, JSNApiHelper::ToJSHandle(exportObject));
        }
        requiredModule->SetException(thread, JSNApiHelper::ToJSHandle(exportObject));
        LOG_FULL(ERROR) << "export objects of native so is undefined, so name is " <<
            requiredModule->GetEcmaModuleRecordNameString();
        return false;
    }
    if (UNLIKELY(exportObject->IsUndefined())) {
        CString fileName = requiredModule->GetEcmaModuleFilenameString();
        LOG_FULL(ERROR) << "export objects of native so is undefined, so name is " <<
            requiredModule->GetEcmaModuleRecordNameString();
        return false;
    }
    ASSERT(!thread->HasPendingException());
    SourceTextModule::StoreModuleValue(thread, requiredModule, 0, JSNApiHelper::ToJSHandle(exportObject));
    return true;
}

bool SourceTextModule::EvaluateNativeModule(JSThread *thread, JSHandle<SourceTextModule> nativeModule,
                                            ModuleTypes moduleType)
{
    if (nativeModule->GetStatus() == ModuleStatus::EVALUATED) {
        return true;
    }
    if (!SourceTextModule::LoadNativeModule(thread, nativeModule, moduleType)) {
        LOG_FULL(INFO) << "LoadNativeModule " << nativeModule->GetEcmaModuleRecordNameString() << " failed";
        // LATER DO: add error status.
        return false;
    }
    nativeModule->SetStatus(ModuleStatus::EVALUATED);
    return true;
}


void SourceTextModule::HandlePreInstantiationException(JSThread *thread, JSHandle<SourceTextModule> module)
{
    if (module->GetStatus() != ModuleStatus::PREINSTANTIATING) {
        return;
    }
    module->SetStatus(ModuleStatus::UNINSTANTIATED);
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            JSHandle<JSTaggedValue> request(thread, requestedModules->Get(idx));
            if (request->IsSourceTextModule()) {
                HandlePreInstantiationException(thread, JSHandle<SourceTextModule>::Cast(request));
            } else if (request->IsString()) {
                ModuleManager *moduleManager = thread->GetModuleManager();
                CString requestStr = ModulePathHelper::Utf8ConvertToString(request.GetTaggedValue());
                JSHandle<SourceTextModule> mm = moduleManager->GetImportedModule(requestStr);
                HandlePreInstantiationException(thread, mm);
            } else {
                // else request is hole;
                break;
            }
        }
    }
}

int SourceTextModule::HandleInstantiateException([[maybe_unused]] JSHandle<SourceTextModule> &module,
                                                 const CVector<JSHandle<SourceTextModule>> &stack, int result)
{
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

int SourceTextModule::Instantiate(JSThread *thread, const JSHandle<JSTaggedValue> &moduleHdl,
    const ExecuteTypes &executeType)
{
    STACK_LIMIT_CHECK(thread, SourceTextModule::UNDEFINED_INDEX);
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SourceTextModule::Instantiate");
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, SourceTextModule::UNDEFINED_INDEX);
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleHdl);
    // 1. Let module be this Source Text Module Record.
    // 2. Assert: module.[[Status]] is one of UNLINKED, LINKED, EVALUATING-ASYNC, or EVALUATED.
    ModuleStatus status = module->GetStatus();
    ASSERT(status == ModuleStatus::UNINSTANTIATED || status == ModuleStatus::INSTANTIATED ||
           status == ModuleStatus::EVALUATING_ASYNC || status == ModuleStatus::EVALUATED ||
           status == ModuleStatus::ERRORED);
    // 4. Let result be InnerModuleInstantiation(module, stack, 0).
    int result = SourceTextModule::PreModuleInstantiation(thread, module, executeType);
    // 5. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        HandlePreInstantiationException(thread, module);
        return result;
    }
    // 3. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    result = FinishModuleInstantiation(thread, module, stack, 0);
    if (thread->HasPendingException()) {
        return HandleInstantiateException(module, stack, result);
    }
    // 6. Assert: module.[[Status]] is one of LINKED, EVALUATING-ASYNC, or EVALUATED.
    status = module->GetStatus();
    ASSERT(status == ModuleStatus::INSTANTIATED || status == ModuleStatus::EVALUATING_ASYNC ||
           status == ModuleStatus::EVALUATED || status == ModuleStatus::ERRORED);
    // 7. Assert: stack is empty.
    ASSERT(stack.empty());
    // 8. Return undefined.
    SharedModuleManager::GetInstance()->TransferSModule(thread);
    return SourceTextModule::UNDEFINED_INDEX;
}

std::optional<std::set<uint32_t>> SourceTextModule::GetConcurrentRequestedModules(const JSHandle<Method> &method)
{
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    const MethodLiteral *methodLiteral = method->GetMethodLiteral();
    ASSERT(methodLiteral != nullptr);
    return methodLiteral->GetConcurrentRequestedModules(jsPandaFile);
}

void SourceTextModule::DFSModuleInstantiation(JSHandle<SourceTextModule> &module,
                                              CVector<JSHandle<SourceTextModule>> &stack)
{
    // 1. Assert: module occurs exactly once in stack.
    // 2. Assert: module.[[DFSAncestorIndex]] is less than or equal to module.[[DFSIndex]].
    int dfsAncIdx = module->GetDFSAncestorIndex();
    int dfsIdx = module->GetDFSIndex();
    ASSERT(dfsAncIdx <= dfsIdx);
    // 3. If module.[[DFSAncestorIndex]] equals module.[[DFSIndex]], then
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
}

bool SourceTextModule::PreModuleInstantiation(JSThread *thread,
    JSHandle<SourceTextModule> module, const ExecuteTypes &executeType)
{
    // Add a safepoint here to check if a suspension is needed.
    thread->CheckSafepointIfSuspended();
    ModuleStatus status = module->GetStatus();
    ASSERT(status != ModuleStatus::INSTANTIATING);
    if (status != ModuleStatus::UNINSTANTIATED && status != ModuleStatus::EVALUATING) {
        return true;
    }
    bool isShared = SourceTextModule::IsSharedModule(module);
    if (isShared && status == ModuleStatus::EVALUATING) {
        LOG_FULL(INFO) << "circular dependency occurred of shared-module";
        return true;
    }
    module->SetStatus(ModuleStatus::PREINSTANTIATING);
    JSHandle<TaggedArray> moduleRequests(thread, module->GetModuleRequests());
    // 9. For each String required that is an element of module.[[RequestedModules]], do
    if (!moduleRequests.GetTaggedValue().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t moduleRequestsLen = moduleRequests->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        JSHandle<TaggedArray> sharedRequestedModules(thread, thread->GlobalConstants()->GetUndefined());
        if (isShared) {
            sharedRequestedModules = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(moduleRequestsLen);
        }
        for (size_t idx = 0; idx < moduleRequestsLen; idx++) {
            required.Update(moduleRequests->Get(idx));
            JSHandle<JSTaggedValue> requiredModule =
                ModuleResolver::HostResolveImportedModule(thread, module, required, executeType);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            if (!isShared) {
                requestedModules->Set(thread, idx, requiredModule.GetTaggedValue());
            } else {
                JSHandle<EcmaString> requireModuleName =
                    thread->GetEcmaVM()->GetFactory()->NewFromUtf8(GetModuleName(requiredModule.GetTaggedValue()));
                requestedModules->Set(thread, idx, requireModuleName.GetTaggedValue());
                sharedRequestedModules->Set(thread, idx, requiredModule.GetTaggedValue());
            }
        }
        // In case of circularImport, we need resolve separately.
        if (isShared) {
            requestedModules = sharedRequestedModules;
        }
        size_t requestedModulesLen = requestedModules->GetLength();
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            JSHandle<SourceTextModule> requestedModule = GetRequestedModule(thread, requestedModules, idx);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            PreModuleInstantiation(thread, requestedModule, executeType);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        }
    }
    return true;
}

int SourceTextModule::FinishModuleInstantiation(JSThread *thread, JSHandle<SourceTextModule> module,
    CVector<JSHandle<SourceTextModule>> &stack, int index)
{
    // Add a safepoint here to check if a suspension is needed.
    thread->CheckSafepointIfSuspended();
    // ArkTS module doesn't implement other module Record, delete follow branch.
    // 1. If module is not a Source Text Module Record, then
        //  a. Perform ? module.Instantiate().
        //  b. Return index.
    // 2. If module.[[Status]] is one of LINKING, LINKED, EVALUATING-ASYNC, or EVALUATED, then Return index.
    ModuleStatus status = module->GetStatus();
    if (status >= ModuleStatus::INSTANTIATING) {
        return index;
    }
    // 3. Assert: module.[[Status]] is "PREINSTANTIATING".
    ASSERT(status == ModuleStatus::PREINSTANTIATING);
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
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            JSHandle<SourceTextModule> requiredModule =
                GetRequestedModule(thread, requestedModules, idx);
            index = FinishModuleInstantiation(thread, requiredModule, stack, index);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
            // c. Assert: requiredModule.[[Status]] is one of LINKING, LINKED, EVALUATING-ASYNC, or EVALUATED.
            ModuleStatus requiredModuleStatus = requiredModule->GetStatus();
            ASSERT(requiredModuleStatus >= ModuleStatus::INSTANTIATING &&
                   requiredModuleStatus != ModuleStatus::EVALUATING);
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
    DFSModuleInstantiation(module, stack);
    return index;
}

void SourceTextModule::ModuleDeclarationEnvironmentSetup(JSThread *thread,
                                                         const JSHandle<SourceTextModule> &module)
{
    CheckResolvedBinding(thread, module);
    if (module->GetImportEntries().IsUndefined()) {
        return;
    }
    ASSERT(!SourceTextModule::IsSharedModule(module));
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
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<ImportEntry> in(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> localName(thread, globalConstants->GetUndefined());
    for (size_t idx = 0; idx < importEntriesLen; idx++) {
        in.Update(importEntries->Get(idx));
        localName.Update(in->GetLocalName());
        importName.Update(in->GetImportName());
        // a. Let importedModule be ! HostResolveImportedModule(module, in.[[ModuleRequest]]).
        JSHandle<SourceTextModule> importedModule = JSHandle<SourceTextModule>::Cast(
            GetRequestedModule(thread, requestedModules, in->GetModuleRequestIndex()));
        RETURN_IF_ABRUPT_COMPLETION(thread);
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
            RETURN_IF_ABRUPT_COMPLETION(thread);
            // ii. If resolution is null or "ambiguous", throw a SyntaxError exception.
            if (resolution->IsNull() || resolution->IsString()) {
                CString requestMod = ModulePathHelper::ReformatPath(GetModuleName(importedModule.GetTaggedValue()));
                CString msg = "the requested module '" + requestMod + GetResolveErrorReason(resolution) +
                    ConvertToString(importName.GetTaggedValue());
                if (!module->GetEcmaModuleRecordNameString().empty()) {
                    CString recordStr = ModulePathHelper::ReformatPath(module->GetEcmaModuleRecordNameString());
                    msg += "' which imported by '" + recordStr + "'";
                } else {
                    msg += "' which imported by '" + module->GetEcmaModuleFilenameString() + "'";
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
    ModuleTraceScope moduleTraceScope(thread,
        "SourceTextModule::Instantiating:" + module->GetEcmaModuleRecordNameString());
    if (IsSharedModule(module) && SharedModuleManager::GetInstance()->IsInstantiatedSModule(thread, module)) {
        return;
    }
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
    // 7. Let envRec be env's EnvironmentRecord.
    JSHandle<TaggedArray> envRec = arr;
    // 8. For each ImportEntry Record in in module.[[ImportEntries]], do
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<ImportEntry> in(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    for (size_t idx = 0; idx < importEntriesLen; idx++) {
        in.Update(importEntries->Get(idx));
        importName.Update(in->GetImportName());
        // a. Let importedModule be ! HostResolveImportedModule(module, in.[[ModuleRequest]]).
        JSHandle<SourceTextModule> importedModule =
            GetRequestedModule(thread, requestedModules, in->GetModuleRequestIndex());
        RETURN_IF_ABRUPT_COMPLETION(thread);
        // c. If in.[[ImportName]] is "*", then
        JSHandle<JSTaggedValue> starString = globalConstants->GetHandledStarString();
        if (JSTaggedValue::SameValue(importName, starString)) {
            // need refactor
            envRec = JSSharedModule::CloneEnvForSModule(thread, module, envRec);
            module->SetEnvironment(thread, envRec);
            return;
        }
        // i. Let resolution be ? importedModule.ResolveExport(in.[[ImportName]], « »).
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveVector;
        JSHandle<JSTaggedValue> resolution =
            SourceTextModule::ResolveExport(thread, importedModule, importName, resolveVector);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        // ii. If resolution is null or "ambiguous", throw a SyntaxError exception.
        if (resolution->IsNull() || resolution->IsString()) {
            CString requestMod = ModulePathHelper::ReformatPath(GetModuleName(importedModule.GetTaggedValue()));
            CString msg = "the requested module '" + requestMod + GetResolveErrorReason(resolution) +
                        ConvertToString(importName.GetTaggedValue());
            if (!module->GetEcmaModuleRecordNameString().empty()) {
                CString recordStr = ModulePathHelper::ReformatPath(
                    module->GetEcmaModuleRecordNameString());
                msg += "' which imported by '" + recordStr + "'";
            } else {
                msg += "' which imported by '" + module->GetEcmaModuleFilenameString() + "'";
            }
            THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
        }
        // iii. Call envRec.CreateImportBinding(
        //    in.[[LocalName]], resolution.[[Module]], resolution.[[BindingName]]).
        envRec->Set(thread, idx, resolution);
    }
    envRec = JSSharedModule::CloneEnvForSModule(thread, module, envRec);
    module->SetEnvironment(thread, envRec);
}

JSHandle<JSTaggedValue> SourceTextModule::GetModuleNamespace(JSThread *thread,
                                                             const JSHandle<SourceTextModule> &module)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Assert: module is an instance of a concrete subclass of Module Record.
    // 2. Assert: module.[[Status]] is not "uninstantiated".
    ASSERT(module->GetStatus() != ModuleStatus::UNINSTANTIATED);
    // 3. Let namespace be module.[[Namespace]].
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
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            // ii. If resolution is a ResolvedBinding Record, append name to unambiguousNames.
            if (resolution->IsModuleBinding()) {
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

void SourceTextModule::HandleEvaluateResult(JSThread *thread, JSHandle<SourceTextModule> &module,
    JSHandle<JSPromise> &capability, const CVector<JSHandle<SourceTextModule>> &stack,
    const CVector<JSHandle<SourceTextModule>> &errorStack)
{
    ModuleStatus status;
    // 9. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        JSHandle<JSTaggedValue> exception(thread, thread->GetException());
        HandleEvaluateException(thread, stack, exception);
        // b. Assert: module.[[Status]] is "evaluated" and module.[[EvaluationError]] is result.
        //d. Perform ! Call(capability.[[Reject]], undefined, « result.[[Value]] »).
        JSPromise::RejectPromise(thread, capability, exception);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        return;
    }
    if (!errorStack.empty()) {
        return HandleErrorStack(thread, errorStack);
    }
    // a. Assert: module.[[Status]] is either EVALUATING-ASYNC or EVALUATED.
    status = module->GetStatus();
    ASSERT(status == ModuleStatus::EVALUATING_ASYNC || status == ModuleStatus::EVALUATED);
    // b. Assert: module.[[EvaluationError]] is EMPTY.
    ASSERT(status != ModuleStatus::ERRORED);
    // c. If module.[[AsyncEvaluation]] is false, then
    //    i. Assert: module.[[Status]] is EVALUATED || ERRORED.
    //    ii. Perform ! Call(capability.[[Resolve]], undefined, « undefined »).
    if (!module->IsAsyncEvaluating()) {
        ASSERT(status >= ModuleStatus::EVALUATED);
    }
    // d. Assert: stack is empty.
    ASSERT(stack.empty());
}

JSTaggedValue SourceTextModule::Evaluate(JSThread *thread, const JSHandle<SourceTextModule> &moduleHdl,
                                         const void *buffer, size_t size, const ExecuteTypes &executeType)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SourceTextModule::Evaluate");
    // 1. Let module be this Source Text Module Record.
    // 2. Assert: module.[[Status]] is one of LINKED, EVALUATING-ASYNC, or EVALUATED.
    JSMutableHandle<SourceTextModule> module(thread, moduleHdl);
    ModuleStatus status = module->GetStatus();
    ASSERT((status == ModuleStatus::INSTANTIATED || status == ModuleStatus::EVALUATING_ASYNC ||
            status == ModuleStatus::EVALUATED));
    // 3. If module.[[Status]] is either EVALUATING-ASYNC or EVALUATED, set module to module.[[CycleRoot]].
    if (status == ModuleStatus::EVALUATING_ASYNC || status == ModuleStatus::EVALUATED) {
        module.Update(module->GetCycleRoot());
    }
    // 4. If module.[[TopLevelCapability]] is not EMPTY, then
    //     a. Return module.[[TopLevelCapability]].[[Promise]].
    // 5. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    CVector<JSHandle<SourceTextModule>> errorStack;
    // 6. Let capability be ! NewPromiseCapability(%Promise%).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSPromise> capability = factory->NewJSPromise();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. Set module.[[TopLevelCapability]] to capability.
    if (!SourceTextModule::IsSharedModule(module)) {
        module->SetTopLevelCapability(thread, capability);
    }

    // 8. Let result be Completion(InnerModuleEvaluation(module, stack, 0)).
    SourceTextModule::InnerModuleEvaluation(thread, module, stack, errorStack, 0, buffer, size, executeType);
    HandleEvaluateResult(thread, module, capability, stack, errorStack);
    if (!thread->HasPendingException() && IsStaticImport(executeType)) {
        job::MicroJobQueue::ExecutePendingJob(thread, thread->GetEcmaVM()->GetMicroJobQueue());
    }
    ModuleLogger *moduleLogger = thread->GetModuleLogger();
    if ((moduleLogger != nullptr) && IsStaticImport(executeType)) {
        moduleLogger->InsertEntryPointModule(module);
    }
    // Return capability.[[Promise]].
    return capability.GetTaggedValue();
}

int SourceTextModule::EvaluateForConcurrent(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                            const JSHandle<Method> &method)
{
    // 1. Let module be this Source Text Module Record.
    // 2. Assert: module.[[Status]] is "instantiated" or "evaluated".
    [[maybe_unused]] ModuleStatus status = module->GetStatus();
    ASSERT((status == ModuleStatus::INSTANTIATED || status == ModuleStatus::EVALUATED));
    // 4. Let result be InnerModuleEvaluation(module, stack, 0)
    int result = SourceTextModule::ModuleEvaluation(thread, module, 0, method);
    // 5. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        return result;
    } else {
        job::MicroJobQueue::ExecutePendingJob(thread, thread->GetEcmaVM()->GetMicroJobQueue());
        return SourceTextModule::UNDEFINED_INDEX;
    }
}

int SourceTextModule::InnerModuleEvaluationUnsafe(JSThread *thread, JSHandle<SourceTextModule> &module,
    CVector<JSHandle<SourceTextModule>> &stack, CVector<JSHandle<SourceTextModule>> &errorStack,
    int index, const void *buffer, size_t size, const ExecuteTypes &executeType)
{
    STACK_LIMIT_CHECK(thread, index);
    ModuleStatus status = module->GetStatus();
    if (status >= ModuleStatus::EVALUATING_ASYNC) {
        if (status == ModuleStatus::ERRORED) {
            ModuleMessageHelper::PrintAndThrowError(thread, module);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        }
        return index;
    }

    if (status == ModuleStatus::EVALUATING) {
        return index;
    }
    ASSERT(status == ModuleStatus::INSTANTIATED);
    module->SetStatus(ModuleStatus::EVALUATING);
    module->SetDFSIndex(index);
    module->SetDFSAncestorIndex(index);
    module->SetPendingAsyncDependencies(0);
    index++;
    stack.emplace_back(module);
    ModuleLogger *moduleLogger = thread->GetModuleLogger();
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSHandle<SourceTextModule> requiredModule;
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            // check if requiredModule is marked lazy
            if (module->IsLazyImportModule(idx)) {
                continue;
            }
            requiredModule = GetRequestedModule(thread, requestedModules, idx);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
            if (moduleLogger != nullptr) {
                moduleLogger->InsertParentModule(module, requiredModule);
            }
            ModuleTypes moduleType = requiredModule->GetTypes();
            if (SourceTextModule::IsNativeModule(moduleType)) {
                EvaluateNativeModule(thread, requiredModule, moduleType);
                continue;
            }
            // if requiredModule is jsonModule, then don't need to execute.
            if (moduleType == ModuleTypes::JSON_MODULE) {
                requiredModule->SetStatus(ModuleStatus::EVALUATED);
                continue;
            }

            index = SourceTextModule::InnerModuleEvaluation(
                thread, requiredModule, stack, errorStack, index, buffer, size, executeType);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
            ModuleStatus requiredModuleStatus = requiredModule->GetStatus();
            ASSERT(requiredModuleStatus >= ModuleStatus::EVALUATING);
            if (requiredModuleStatus == ModuleStatus::EVALUATING) {
                ASSERT(std::find(stack.begin(), stack.end(), requiredModule) != stack.end());
            }
            if (std::find(stack.begin(), stack.end(), requiredModule) != stack.end()) {
                ASSERT(requiredModuleStatus == ModuleStatus::EVALUATING);
            }

            if (requiredModuleStatus == ModuleStatus::EVALUATING) {
                int dfsAncIdx = std::min(module->GetDFSAncestorIndex(), requiredModule->GetDFSAncestorIndex());
                module->SetDFSAncestorIndex(dfsAncIdx);
            } else {
                requiredModule = JSHandle<SourceTextModule>(thread, requiredModule->GetCycleRoot());
                requiredModuleStatus = requiredModule->GetStatus();
                ASSERT(requiredModuleStatus >= ModuleStatus::EVALUATING_ASYNC);
                if (requiredModuleStatus == ModuleStatus::ERRORED) {
                    errorStack.emplace_back(module);
                    SetExceptionToModule(thread, module, requiredModule->GetException());
                    ModuleMessageHelper::PrintAndThrowError(thread, module);
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
                    return index;
                }
            }
            if (requiredModule->IsAsyncEvaluating()) {
                module->SetPendingAsyncDependencies(module->GetPendingAsyncDependencies() + 1);
                AddAsyncParentModule(thread, requiredModule, module);
            }
        }
    }
    int pendingAsyncDependencies = module->GetPendingAsyncDependencies();
    bool hasTLA = module->GetHasTLA();
    // 12. If module.[[PendingAsyncDependencies]] > 0 or module.[[HasTLA]] is true, then
    if (pendingAsyncDependencies > 0 || hasTLA) {
        // a. Assert: module.[[AsyncEvaluation]] is false and was never previously set to true.
        ASSERT(module->GetAsyncEvaluatingOrdinal() == NOT_ASYNC_EVALUATED);
        // b. Set module.[[AsyncEvaluation]] to true.
        auto moduleManager = thread->GetModuleManager();
        module->SetAsyncEvaluatingOrdinal(moduleManager->NextModuleAsyncEvaluatingOrdinal());
        // d. If module.[[PendingAsyncDependencies]] = 0, perform ExecuteAsyncModule(module).
        if (pendingAsyncDependencies == 0) {
            SourceTextModule::ExecuteAsyncModule(thread, module, buffer, size, executeType);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        }
    } else {
        // 13. Else, Perform ? module.ExecuteModule().
        SourceTextModule::ModuleExecution(thread, module, buffer, size, executeType);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
    }
    // 14. Assert: module occurs exactly once in stack.
    // 15. Assert: module.[[DFSAncestorIndex]] ≤ module.[[DFSIndex]].
    int dfsAncIdx = module->GetDFSAncestorIndex();
    int dfsIdx = module->GetDFSIndex();
    ASSERT(dfsAncIdx <= dfsIdx);
    // 16. If module.[[DFSAncestorIndex]] = module.[[DFSIndex]], then
    if (dfsAncIdx == dfsIdx) {
        // a. Let done be false.
        bool done = false;
        // b. Repeat, while done is false,
        while (!done) {
            // i. Let requiredModule be the last element in stack.
            JSHandle<SourceTextModule> requiredModule = stack.back();
            // ii. Remove the last element of stack.
            stack.pop_back();
            // iii. Assert: requiredModule is a Cyclic Module Record.
            // iv. If requiredModule.[[AsyncEvaluation]] is false, set requiredModule.[[Status]] to EVALUATED.
            // v. Otherwise, set requiredModule.[[Status]] to EVALUATING-ASYNC.
            if (!requiredModule->IsAsyncEvaluating()) {
                requiredModule->SetStatus(ModuleStatus::EVALUATED);
            } else {
                requiredModule->SetStatus(ModuleStatus::EVALUATING_ASYNC);
            }
            // vi. If requiredModule and module are the same Module Record, set done to true.
            if (JSTaggedValue::SameValue(module.GetTaggedValue(), requiredModule.GetTaggedValue())) {
                done = true;
            }
            // vii. Set requiredModule.[[CycleRoot]] to module.
            if (!SourceTextModule::IsSharedModule(requiredModule)) {
                requiredModule->SetCycleRoot(thread, module);
            }
        }
    }
    return index;
}

bool SourceTextModule::IsEvaluatedModule(JSThread *thread, StateVisit &stateVisit,
                                         const JSHandle<SourceTextModule> &module)
{
    // Status maybe EVALUATED || ERRORED.
    return GetModuleEvaluatingType(thread, stateVisit, module) >= ModuleStatus::EVALUATED;
}

ModuleStatus SourceTextModule::GetModuleEvaluatingType(JSThread *thread, StateVisit &stateVisit,
                                                       const JSHandle<SourceTextModule> &module)
{
    RuntimeLockHolder locker(thread, stateVisit.mutex);
    return module->GetStatus();
}

int SourceTextModule::InnerModuleEvaluation(JSThread *thread, JSHandle<SourceTextModule> &module,
                                            CVector<JSHandle<SourceTextModule>> &stack,
                                            CVector<JSHandle<SourceTextModule>> &errorStack,
                                            int index, const void *buffer, size_t size, const ExecuteTypes &executeType)
{
    bool isShared = IsSharedModule(module);
    if (!isShared) {
        return SourceTextModule::InnerModuleEvaluationUnsafe(
            thread, module, stack, errorStack, index, buffer, size, executeType);
    } else {
        StateVisit &stateVisit = SharedModuleManager::GetInstance()->findModuleMutexWithLock(thread, module);
        ModuleStatus status = module->GetStatus();
        if (status == ModuleStatus::EVALUATING &&
                stateVisit.threadId == thread->GetThreadId()) {
            return index;
        }
        if (status == ModuleStatus::ERRORED) {
            ModuleMessageHelper::PrintAndThrowError(thread, module);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
            return index;
        }
        RuntimeLockHolder locker(thread, stateVisit.mutex);
        if (status == ModuleStatus::INSTANTIATED) {
            stateVisit.threadId = thread->GetThreadId();
            int idx = SourceTextModule::InnerModuleEvaluationUnsafe(
                thread, module, stack, errorStack, index, buffer, size, executeType);
            return idx;
        }
        return index;
    }
    LOG_FULL(FATAL) << "This line is unreachable";
    UNREACHABLE();
}

void SourceTextModule::HandleConcurrentEvaluateResult(JSThread *thread, JSHandle<SourceTextModule> &module,
    const CVector<JSHandle<SourceTextModule>> &stack, const CVector<JSHandle<SourceTextModule>> &errorStack)
{
    ModuleStatus status;
    // 9. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        JSHandle<JSTaggedValue> exception(thread, thread->GetException());
        HandleEvaluateException(thread, stack, exception);
        // b. Assert: module.[[Status]] is "evaluated" and module.[[EvaluationError]] is result.
        return;
    }
    if (!errorStack.empty()) {
        return HandleErrorStack(thread, errorStack);
    }
    // a. Assert: module.[[Status]] is either EVALUATING-ASYNC or EVALUATED.
    status = module->GetStatus();
    ASSERT(status == ModuleStatus::EVALUATING_ASYNC ||
           status == ModuleStatus::EVALUATED ||
           status != ModuleStatus::ERRORED);
    // c. If module.[[AsyncEvaluation]] is false, then
    //    i. Assert: module.[[Status]] is EVALUATED || ERRORED.
    if (!module->IsAsyncEvaluating()) {
        ASSERT(status >= ModuleStatus::EVALUATED);
    }
    // d. Assert: stack is empty.
    ASSERT(stack.empty());
}

int SourceTextModule::ModuleEvaluation(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                       int index, const JSHandle<Method> &method)
{
    if (!module->GetRequestedModules().IsUndefined()) {
        JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
        size_t requestedModulesLen = requestedModules->GetLength();
        JSMutableHandle<JSTaggedValue> required(thread, thread->GlobalConstants()->GetUndefined());
        auto coRequestedModules = GetConcurrentRequestedModules(method);
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            if (coRequestedModules.has_value() && coRequestedModules.value().count(idx) == 0) {
                // skip the unused module
                continue;
            }
            JSHandle<SourceTextModule> requiredModule = GetRequestedModule(thread, requestedModules, idx);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);

            ModuleTypes moduleType = requiredModule->GetTypes();
            if (SourceTextModule::IsNativeModule(moduleType)) {
                EvaluateNativeModule(thread, requiredModule, moduleType);
                continue;
            }
            if (moduleType == ModuleTypes::JSON_MODULE) {
                requiredModule->SetStatus(ModuleStatus::EVALUATED);
                continue;
            }
            CVector<JSHandle<SourceTextModule>> stack;
            CVector<JSHandle<SourceTextModule>> errorStack;
            int result = SourceTextModule::InnerModuleEvaluation(thread, requiredModule, stack, errorStack, 0);
            index += result;
            HandleConcurrentEvaluateResult(thread, requiredModule, stack, errorStack);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        }
    }
    return index;
}

Expected<JSTaggedValue, bool> SourceTextModule::ModuleExecution(JSThread *thread,
    const JSHandle<SourceTextModule> &module, const void *buffer, size_t size, const ExecuteTypes &executeType)
{
    CString moduleFilenameStr {};
    if (thread->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN) {
        moduleFilenameStr = thread->GetEcmaVM()->GetQuickFixManager()->GetBaseFileName(module);
    } else {
        moduleFilenameStr = module->GetEcmaModuleFilenameString();
    }
    std::string entryPoint;
    CString moduleRecordName = module->GetEcmaModuleRecordNameString();
    if (moduleRecordName.empty()) {
        entryPoint = JSPandaFile::ENTRY_FUNCTION_NAME;
    } else {
        entryPoint = moduleRecordName;
    }

    std::shared_ptr<JSPandaFile> jsPandaFile;
    if (buffer != nullptr) {
        jsPandaFile =
            JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFilenameStr, entryPoint, buffer, size);
    } else {
        jsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(
            thread, moduleFilenameStr, entryPoint, false, executeType);
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, Unexpected(false));
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " <<  moduleFilenameStr;
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile.get(), entryPoint, executeType);
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
    JSTaggedValue dictionary = GetNameDictionary();
    if (dictionary.IsUndefined()) {
        // if module is Errored, throw origin jsError
        this->CheckAndThrowModuleError(thread);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        if (isThrow) {
            CString errorMsg = GetEcmaModuleRecordNameString();
            errorMsg = errorMsg.empty() ? GetEcmaModuleFilenameString() :
                                          errorMsg;
            errorMsg.append(" environment is undefined");
            THROW_REFERENCE_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }
        return JSTaggedValue::Hole();
    }

    TaggedArray *array = TaggedArray::Cast(dictionary.GetTaggedObject());
    return array->Get(index);
}

JSTaggedValue SourceTextModule::GetModuleValue(JSThread *thread, JSTaggedValue key, bool isThrow)
{
    JSTaggedValue dictionary = GetNameDictionary();
    if (dictionary.IsUndefined()) {
        // if module is Errored, throw origin jsError
        this->CheckAndThrowModuleError(thread);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        if (isThrow) {
            CString errorMsg = GetEcmaModuleRecordNameString();
            errorMsg = errorMsg.empty() ? GetEcmaModuleFilenameString() :
                                          errorMsg;
            errorMsg.append(" environment is undefined");
            THROW_REFERENCE_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
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

JSTaggedValue SourceTextModule::GetValueFromExportObject(JSThread *thread, JSHandle<JSTaggedValue> &exportObject,
    int32_t index)
{
    if (index == SourceTextModule::UNDEFINED_INDEX) {
        return exportObject.GetTaggedValue();
    }
    JSTaggedValue value = JSTaggedValue::Hole();
    JSObject *obj = JSObject::Cast(exportObject.GetTaggedValue());
    TaggedArray *properties = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!properties->IsDictionaryMode()) {
        JSHClass *jsHclass = obj->GetJSHClass();
        LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
        PropertyAttributes attr = layoutInfo->GetAttr(index);
        value = obj->GetProperty(jsHclass, attr);
    } else {
        NameDictionary *dict = NameDictionary::Cast(properties);
        value = dict->GetValue(index);
    }
    if (UNLIKELY(value.IsAccessor())) {
        return FastRuntimeStub::CallGetter(thread, JSTaggedValue(obj), JSTaggedValue(obj), value);
    }
    return value;
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

// static
void SourceTextModule::StoreModuleValue(JSThread *thread, const JSHandle<SourceTextModule> &module, int32_t index,
                                        const JSHandle<JSTaggedValue> &value)
{
    if (UNLIKELY(IsSharedModule(module)) && !value->IsSharedType()) {
        CString msg = "Export non-shared object from shared-module, module name is :" +
                    module->GetEcmaModuleRecordNameString();
        THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
    }
    JSTaggedValue localExportEntries = module->GetLocalExportEntries();
    ASSERT(localExportEntries.IsTaggedArray());

    JSHandle<JSTaggedValue> data(thread, module->GetNameDictionary());
    if (data->IsUndefined()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        uint32_t size = TaggedArray::Cast(localExportEntries.GetTaggedObject())->GetLength();
        ASSERT(index < static_cast<int32_t>(size));
        if (SourceTextModule::IsSharedModule(module)) {
            data = JSHandle<JSTaggedValue>(factory->NewSTaggedArray(size,
                JSTaggedValue::Hole(), MemSpaceType::SHARED_OLD_SPACE));
        } else {
            data = JSHandle<JSTaggedValue>(factory->NewTaggedArray(size));
        }
        module->SetNameDictionary(thread, data);
    }
    JSHandle<TaggedArray> arr(data);
    arr->Set(thread, index, value);
}

// static
// discard instructions won't consider shared-module.
void SourceTextModule::StoreModuleValue(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                        const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    if (UNLIKELY(IsSharedModule(module)) && !value->IsSharedType()) {
        CString msg = "Export non-shared object from shared-module, module name is :" +
                    module->GetEcmaModuleRecordNameString();
        THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
    }
    JSMutableHandle<JSTaggedValue> data(thread, module->GetNameDictionary());
    if (data->IsUndefined()) {
        data.Update(NameDictionary::Create(thread, DEFAULT_DICTIONART_CAPACITY));
    }
    JSHandle<NameDictionary> dataDict = JSHandle<NameDictionary>::Cast(data);
    data.Update(NameDictionary::Put(thread, dataDict, key, value, PropertyAttributes::Default()));

    module->SetNameDictionary(thread, data);
}

void SourceTextModule::SetExportName(JSThread *thread, const JSHandle<SourceTextModule> requestedModule,
                                     CVector<std::string> &exportedNames, JSHandle<TaggedArray> &newExportStarSet)

{
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
                                                            const JSHandle<SourceTextModule> importedModule,
                                                            JSMutableHandle<JSTaggedValue> &starResolution,
                                                            CVector<std::pair<JSHandle<SourceTextModule>,
                                                            JSHandle<JSTaggedValue>>> &resolveVector)
{
    // b. Let resolution be ? importedModule.ResolveExport(exportName, resolveVector).
    auto moduleType = importedModule->GetTypes();
    bool isNativeModule = IsNativeModule(moduleType);
    JSHandle<JSTaggedValue> resolution;
    if (UNLIKELY(isNativeModule || moduleType == ModuleTypes::CJS_MODULE)) {
        resolution = isNativeModule
            ? SourceTextModule::ResolveNativeStarExport(thread, importedModule, exportName)
            : SourceTextModule::ResolveCjsStarExport(thread, importedModule, exportName);
    } else {
        resolution = SourceTextModule::ResolveExport(thread, importedModule, exportName, resolveVector);
    }

    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // c. If resolution is "ambiguous", return "ambiguous".
    auto globalConstants = thread->GlobalConstants();
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
        if (IsNativeModule(moduleType) || moduleType == ModuleTypes::CJS_MODULE) {
            return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedBindingRecord(module, exportName));
        }

        if ((JSTaggedValue::SameValue(ee->GetExportName(), exportName.GetTaggedValue()))) {
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
    JSTaggedValue undefined = globalConstants->GetUndefined();
    JSMutableHandle<IndirectExportEntry> ee(thread, undefined);
    JSMutableHandle<JSTaggedValue> importName(thread, undefined);
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
    JSHandle<TaggedArray> indirectExportEntries(exportEntry);
    size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
    for (size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
        ee.Update(indirectExportEntries->Get(idx));
        //  a. If SameValue(exportName, e.[[ExportName]]) is true, then
        if (JSTaggedValue::SameValue(exportName.GetTaggedValue(), ee->GetExportName())) {
            // i. Assert: module imports a specific binding for this export.
            // ii. Let importedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
            JSHandle<SourceTextModule> requestedModule =
                GetRequestedModule(thread, requestedModules, ee->GetModuleRequestIndex());
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
        RETURN_IF_ABRUPT_COMPLETION(thread);
        // b. If resolution is null or "ambiguous", throw a SyntaxError exception.
        if (resolution->IsNull() || resolution->IsString()) {
            TaggedArray *requestArray = TaggedArray::Cast(module->GetModuleRequests().GetTaggedObject());
            JSTaggedValue moduleRequest = requestArray->Get(ee->GetModuleRequestIndex());
            CString requestMod = ModulePathHelper::ReformatPath(ConvertToString(moduleRequest));
            CString msg = "the requested module '" + requestMod + GetResolveErrorReason(resolution) +
                          ConvertToString(exportName.GetTaggedValue());
            if (!module->GetEcmaModuleRecordNameString().empty()) {
            CString recordStr = ModulePathHelper::ReformatPath(module->GetEcmaModuleRecordNameString());
                msg += "' which exported by '" + recordStr + "'";
            } else {
                msg += "' which exported by '" + module->GetEcmaModuleFilenameString() + "'";
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
        RETURN_IF_ABRUPT_COMPLETION(thread);
        // b. If resolution is null or "ambiguous", throw a SyntaxError exception.
        if (resolution->IsNull() || resolution->IsString()) {
            TaggedArray *requestArray = TaggedArray::Cast(module->GetModuleRequests().GetTaggedObject());
            JSTaggedValue moduleRequest = requestArray->Get(ee->GetModuleRequestIndex());
            CString requestMod = ModulePathHelper::ReformatPath(ConvertToString(moduleRequest));
            CString msg = "the requested module '" + requestMod + GetResolveErrorReason(resolution) +
                ConvertToString(exportName.GetTaggedValue());
            if (!module->GetEcmaModuleRecordNameString().empty()) {
                CString record = ModulePathHelper::ReformatPath(module->GetEcmaModuleRecordNameString());
                msg += "' which exported by '" + record + "'";
            } else {
                msg += "' which exported by '" + module->GetEcmaModuleFilenameString() + "'";
            }
            THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, msg.c_str());
        }
    }
}

CString SourceTextModule::GetModuleName(JSTaggedValue currentModule)
{
    SourceTextModule *module = SourceTextModule::Cast(currentModule.GetTaggedObject());
    CString recordName = module->GetEcmaModuleRecordNameString();
    if (recordName.empty()) {
        recordName = module->GetEcmaModuleFilenameString();
    }
    return recordName;
}

bool SourceTextModule::IsDynamicModule(LoadingTypes types)
{
    return types == LoadingTypes::DYNAMITC_MODULE;
}

bool SourceTextModule::IsAsyncEvaluating()
{
    return GetAsyncEvaluatingOrdinal() >= FIRST_ASYNC_EVALUATING_ORDINAL;
}

void SourceTextModule::AddAsyncParentModule(JSThread *thread, JSHandle<SourceTextModule> &module,
                                            JSHandle<SourceTextModule> &parent)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue asyncParentModules = module->GetAsyncParentModules();
    if (asyncParentModules.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(1);
        array->Set(thread, 0, parent.GetTaggedValue());
        module->SetAsyncParentModules(thread, array);
    } else {
        JSHandle<TaggedArray> array(thread, asyncParentModules);
        ASSERT(array->GetLength() > 0);
        array = TaggedArray::SetCapacity(thread, array, array->GetLength() + 1);
        array->Set(thread, array->GetLength() - 1, parent.GetTaggedValue());
        module->SetAsyncParentModules(thread, array);
    }
}

void SourceTextModule::ExecuteAsyncModule(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                          const void *buffer, size_t size, const ExecuteTypes &executeType)
{
    // 1. Assert: module.[[Status]] is either EVALUATING or EVALUATING-ASYNC.
    ASSERT(module->GetStatus() == ModuleStatus::EVALUATING || module->GetStatus() == ModuleStatus::EVALUATING_ASYNC);
    // 2. Assert: module.[[HasTLA]] is true.
    ASSERT(module->GetHasTLA());
    CString moduleFilenameStr = module->GetEcmaModuleFilenameString();

    std::string entryPoint;
    CString moduleRecordName = module->GetEcmaModuleRecordNameString();
    if (moduleRecordName.empty()) {
        entryPoint = JSPandaFile::ENTRY_FUNCTION_NAME;
    } else {
        entryPoint = moduleRecordName;
    }

    std::shared_ptr<JSPandaFile> jsPandaFile;
    if (buffer != nullptr) {
        jsPandaFile =
            JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFilenameStr, entryPoint, buffer, size);
    } else {
        jsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(
            thread, moduleFilenameStr, entryPoint, false, executeType);
    }
    RETURN_IF_ABRUPT_COMPLETION(thread);
    if (jsPandaFile == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "Load current file's panda file failed. Current file is " << moduleFilenameStr;
    }
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::Execute(thread, jsPandaFile.get(), entryPoint, executeType);
    ASSERT(result.Value().IsJSPromise());
    // 3. Let capability be ! NewPromiseCapability(%Promise%).
    // 4. Let fulfilledClosure be a new Abstract Closure with no parameters that captures module and performs
    //    the following steps when called:
    //    a. Perform AsyncModuleExecutionFulfilled(module).
    //    b. Return undefined.
    // 5. Let onFulfilled be CreateBuiltinFunction(fulfilledClosure, 0, "", « »).
    // 6. Let rejectedClosure be a new Abstract Closure with parameters (error) that captures module and performs
    //    the following steps when called:
    //    a. Perform AsyncModuleExecutionRejected(module, error).
    //    b. Return undefined.
    // 7. Let onRejected be CreateBuiltinFunction(rejectedClosure, 0, "", « »).
    // 8. Perform PerformPromiseThen(capability.[[Promise]], onFulfilled, onRejected).
    JSHandle<JSPromise> promise(thread, result.Value());
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAsyncModuleFulfilledFunction> onFulfilled =
                    factory->CreateJSAsyncModuleFulfilledFunction();
    onFulfilled->SetModule(thread, module);

    JSHandle<JSAsyncModuleRejectedFunction> onRejected =
                    factory->CreateJSAsyncModuleRejectedFunction();
    onRejected->SetModule(thread, module);
    JSHandle<PromiseCapability> tcap =
                    JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));
    RETURN_IF_ABRUPT_COMPLETION(thread);
    builtins::BuiltinsPromise::PerformPromiseThen(
        thread, promise, JSHandle<JSTaggedValue>::Cast(onFulfilled),
        JSHandle<JSTaggedValue>::Cast(onRejected), tcap);
}

void SourceTextModule::GatherAvailableAncestors(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                                AsyncParentCompletionSet &execList)
{
    auto globalConstants = thread->GlobalConstants();
    JSTaggedValue asyncParentModulesValue = module->GetAsyncParentModules();
    if (asyncParentModulesValue.IsUndefined()) {
        return;
    }
    JSMutableHandle<SourceTextModule> cycleRoot(thread, globalConstants->GetUndefined());
    JSHandle<TaggedArray> asyncParentModules(thread, asyncParentModulesValue);
    size_t asyncParentModulesLen = asyncParentModules->GetLength();
    // 1. For each Cyclic Module Record m of module.[[AsyncParentModules]], do
    for (size_t idx = 0; idx < asyncParentModulesLen; idx++) {
        JSHandle<SourceTextModule> parentModule(thread, asyncParentModules->Get(idx));
        // a. If execList does not contain m and m.[[CycleRoot]].[[EvaluationError]] is EMPTY, then
        cycleRoot.Update(parentModule->GetCycleRoot());
        if (execList.find(parentModule) == execList.end() &&
            cycleRoot->GetStatus() != ModuleStatus::ERRORED) {
            // i. Assert: m.[[Status]] is EVALUATING-ASYNC.
            ASSERT(parentModule->GetStatus() == ModuleStatus::EVALUATING_ASYNC);
            // ii. Assert: m.[[EvaluationError]] is EMPTY.
            // iii. Assert: m.[[AsyncEvaluation]] is true.
            ASSERT(parentModule->IsAsyncEvaluating());
            // iv. Assert: m.[[PendingAsyncDependencies]] > 0.
            ASSERT(parentModule->GetPendingAsyncDependencies() > 0);
            // v. Set m.[[PendingAsyncDependencies]] to m.[[PendingAsyncDependencies]] - 1.
            parentModule->SetPendingAsyncDependencies(parentModule->GetPendingAsyncDependencies() - 1);
            // vi. If m.[[PendingAsyncDependencies]] = 0, then
            //     1. Append m to execList.
            //     2. If m.[[HasTLA]] is false, perform GatherAvailableAncestors(m, execList).
            if (parentModule->GetPendingAsyncDependencies() == 0) {
                execList.insert(parentModule);
                if (!parentModule->GetHasTLA()) {
                    GatherAvailableAncestors(thread, parentModule, execList);
                }
            }
        }
    }
}

void SourceTextModule::AsyncModuleExecutionFulfilled(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    // 1. If module.[[Status]] is EVALUATED, then
    //    a. Assert: module.[[EvaluationError]] is not EMPTY.
    //    b. Return UNUSED.
    if (module->GetStatus() == ModuleStatus::ERRORED) {
        ASSERT(!module->GetException().IsHole());
        return;
    }
    // 2. Assert: module.[[Status]] is EVALUATING-ASYNC.
    ASSERT(module->GetStatus() == ModuleStatus::EVALUATING_ASYNC);
    // 3. Assert: module.[[AsyncEvaluation]] is true.
    ASSERT(module->IsAsyncEvaluating());
    // 4. Assert: module.[[EvaluationError]] is EMPTY.
    // 5. Set module.[[AsyncEvaluation]] to false.
    module->SetAsyncEvaluatingOrdinal(ASYNC_EVALUATE_DID_FINISH);
    // 6. Set module.[[Status]] to EVALUATED.
    module->SetStatus(ModuleStatus::EVALUATED);
    // 7. If module.[[TopLevelCapability]] is not EMPTY, then
    //    a. Assert: module.[[CycleRoot]] is module.
    //    b. Perform ! Call(module.[[TopLevelCapability]].[[Resolve]], undefined, « undefined »).
    auto globalConstants = thread->GlobalConstants();
    JSTaggedValue topLevelCapabilityValue = module->GetTopLevelCapability();
    if (!topLevelCapabilityValue.IsUndefined()) {
        ASSERT(JSTaggedValue::SameValue(module->GetCycleRoot(), module.GetTaggedValue()));
        JSHandle<JSPromise> topLevelCapability(thread, JSPromise::Cast(topLevelCapabilityValue.GetTaggedObject()));
        JSHandle<JSTaggedValue> undefinedValue = globalConstants->GetHandledUndefined();
        JSPromise::FulfillPromise(thread, topLevelCapability, undefinedValue);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
    // 8. Let execList be a new empty List.
    AsyncParentCompletionSet execList;
    // 9. Perform GatherAvailableAncestors(module, execList).
    // 10. Let sortedExecList be a List whose elements are the elements of execList,
    //     in the order in which they had their [[AsyncEvaluation]] fields set to true in InnerModuleEvaluation.
    GatherAvailableAncestors(thread, module, execList);
    // 11. Assert: All elements of sortedExecList have their [[AsyncEvaluation]] field set to true,
    //     [[PendingAsyncDependencies]] field set to 0, and [[EvaluationError]] field set to EMPTY.
    // 12. For each Cyclic Module Record m of sortedExecList, do
    for (JSHandle<SourceTextModule> m : execList) {
        // a. If m.[[Status]] is EVALUATED, then
        //    i. Assert: m.[[EvaluationError]] is not EMPTY.
        if (!m->IsAsyncEvaluating()) {
            ASSERT(module->GetStatus() == ModuleStatus::ERRORED);
        // b. Else if m.[[HasTLA]] is true, then
        //    i. Perform ExecuteAsyncModule(m).
        } else if (m->GetHasTLA()) {
            ExecuteAsyncModule(thread, m);
        // c. Else,
        } else {
            // i. Let result be m.ExecuteModule().
            Expected<JSTaggedValue, bool> result = SourceTextModule::ModuleExecution(thread, m);
            // ii. If result is an abrupt completion, then
            //     1. Perform AsyncModuleExecutionRejected(m, result.[[Value]]).
            if (thread->HasPendingException() || !result || result.Value().IsException()) {
                AsyncModuleExecutionRejected(thread, m, JSTaggedValue::Exception());
            // iii. Else,
            } else {
                // 1. Set m.[[Status]] to EVALUATED.
                m->SetStatus(ModuleStatus::EVALUATED);
                // 2. If m.[[TopLevelCapability]] is not EMPTY, then
                //    a. Assert: m.[[CycleRoot]] is m.
                //    b. Perform ! Call(m.[[TopLevelCapability]].[[Resolve]], undefined, « undefined »).
                JSTaggedValue capabilityValue = m->GetTopLevelCapability();
                if (!capabilityValue.IsUndefined()) {
                    ASSERT(JSTaggedValue::SameValue(m->GetCycleRoot(), m.GetTaggedValue()));
                    JSHandle<JSPromise> topLevelCapability(thread,
                        JSPromise::Cast(topLevelCapabilityValue.GetTaggedObject()));
                    JSHandle<JSTaggedValue> undefinedValue = globalConstants->GetHandledUndefined();
                    JSPromise::FulfillPromise(thread, topLevelCapability, undefinedValue);
                    RETURN_IF_ABRUPT_COMPLETION(thread);
                }
            }
        }
    }
}

void SourceTextModule::AsyncModuleExecutionRejected(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                                    JSTaggedValue error)
{
    // 1. If module.[[Status]] is EVALUATED, then
    //    a. Assert: module.[[EvaluationError]] is not EMPTY.
    //    b. Return UNUSED.
    if (module->GetStatus() == ModuleStatus::ERRORED) {
        ASSERT(!module->GetException().IsHole());
        return;
    }
    // 2. Assert: module.[[Status]] is EVALUATING-ASYNC.
    ASSERT(module->GetStatus() == ModuleStatus::EVALUATING_ASYNC);
    // 3. Assert: module.[[AsyncEvaluation]] is true.
    ASSERT(module->IsAsyncEvaluating());
    // 4. Assert: module.[[EvaluationError]] is EMPTY.
    ASSERT(module->GetException().IsHole());
    // 5. Set module.[[EvaluationError]] to ThrowCompletion(error).
    module->SetStatus(ModuleStatus::ERRORED);
    // 6. Set module.[[Status]] to EVALUATED.
    module->SetException(thread, error);
    // 7. For each Cyclic Module Record m of module.[[AsyncParentModules]], do
    //    a. Perform AsyncModuleExecutionRejected(m, error).
    auto globalConstants = thread->GlobalConstants();
    JSTaggedValue asyncParentModulesValue = module->GetAsyncParentModules();
    if (!asyncParentModulesValue.IsUndefined()) {
        JSMutableHandle<SourceTextModule> parentModule(thread, globalConstants->GetUndefined());
        JSHandle<TaggedArray> asyncParentModules(thread, asyncParentModulesValue);
        size_t asyncParentModulesLen = asyncParentModules->GetLength();
        for (size_t idx = 0; idx < asyncParentModulesLen; idx++) {
            parentModule.Update(asyncParentModules->Get(idx));
            AsyncModuleExecutionRejected(thread, parentModule, error);
        }
    }

    // 8. If module.[[TopLevelCapability]] is not EMPTY, then
    //    a. Assert: module.[[CycleRoot]] is module.
    //    b. Perform ! Call(module.[[TopLevelCapability]].[[Reject]], undefined, « error »).
    JSTaggedValue topLevelCapabilityValue = module->GetTopLevelCapability();
    if (!topLevelCapabilityValue.IsUndefined()) {
        JSHandle<JSTaggedValue> exceptionHandle(thread, error);
        // if caught exceptionHandle type is JSError
        if (exceptionHandle->IsJSError()) {
            thread->HandleUncaughtException(error);
        }
        ASSERT(JSTaggedValue::SameValue(module->GetCycleRoot(), module.GetTaggedValue()));
        JSHandle<JSPromise> topLevelCapability(thread, JSPromise::Cast(topLevelCapabilityValue.GetTaggedObject()));
        JSPromise::RejectPromise(thread, topLevelCapability, exceptionHandle);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
}

JSTaggedValue SourceTextModule::AsyncModuleFulfilledFunc(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSAsyncModuleFulfilledFunction> fulfilledFunc =
        JSHandle<JSAsyncModuleFulfilledFunction>::Cast(base::BuiltinsBase::GetConstructor(argv));
    JSHandle<SourceTextModule> module(thread, fulfilledFunc->GetModule());
    AsyncModuleExecutionFulfilled(thread, module);
    return JSTaggedValue::Undefined();
}

JSTaggedValue SourceTextModule::AsyncModuleRejectedFunc(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSAsyncModuleRejectedFunction> rejectedFunc =
        JSHandle<JSAsyncModuleRejectedFunction>::Cast(base::BuiltinsBase::GetConstructor(argv));
    JSHandle<SourceTextModule> module(thread, rejectedFunc->GetModule());
    [[maybe_unused]] JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, 0);
    AsyncModuleExecutionRejected(thread, module, value.GetTaggedValue());
    return JSTaggedValue::Undefined();
}

void SourceTextModule::CheckCircularImportTool(JSThread *thread, const CString &circularModuleRecordName,
                                               CList<CString> &referenceList, bool printOtherCircular)
{
    referenceList.push_back(circularModuleRecordName);
    JSMutableHandle<SourceTextModule> moduleRecord(thread, thread->GlobalConstants()->GetUndefined());
    auto moduleManager = thread->GetModuleManager();
    if (moduleManager->IsLocalModuleLoaded(circularModuleRecordName)) {
        moduleRecord.Update(moduleManager->HostGetImportedModule(circularModuleRecordName));
    } else {
        moduleRecord.Update(ModuleResolver::HostResolveImportedModule(thread, circularModuleRecordName));
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
    CString requiredModuleName;
    SourceTextModule::SearchCircularImport(
        thread, circularModuleRecordName, moduleRecord, referenceList, requiredModuleName, printOtherCircular);
}

void SourceTextModule::SearchCircularImport(JSThread *thread, const CString &circularModuleRecordName,
                                            const JSHandle<SourceTextModule> &module,
                                            CList<CString> &referenceList,
                                            CString &requiredModuleName, bool printOtherCircular)
{
    if (module->GetModuleRequests().IsUndefined()) {
        return;
    }
    auto globalConstants = thread->GlobalConstants();
    JSHandle<TaggedArray> moduleRequests(thread, module->GetModuleRequests());
    size_t moduleRequestsLen = moduleRequests->GetLength();
    JSMutableHandle<JSTaggedValue> required(thread, globalConstants->GetUndefined());
    JSMutableHandle<SourceTextModule> requiredModule(thread, globalConstants->GetUndefined());
    for (size_t idx = 0; idx < moduleRequestsLen; idx++) {
        required.Update(moduleRequests->Get(idx));
        requiredModule.Update(JSHandle<SourceTextModule>::Cast(
            ModuleResolver::HostResolveImportedModule(thread, module, required)));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        requiredModuleName = requiredModule->GetEcmaModuleRecordNameString();
        referenceList.push_back(requiredModuleName);
        if (requiredModuleName == circularModuleRecordName) {
            PrintCircular(referenceList, ERROR);
        } else if (printOtherCircular && IsCircular(referenceList, requiredModuleName)) {
            PrintCircular(referenceList, WARN);
        } else {
            SourceTextModule::SearchCircularImport(thread, circularModuleRecordName,
                requiredModule, referenceList, requiredModuleName, printOtherCircular);
        }
        referenceList.pop_back();
    }
}

bool SourceTextModule::IsCircular(const CList<CString> &referenceList,
                                  const CString &requiredModuleName)
{
    for (auto iter = referenceList.begin(), end = --referenceList.end(); iter != end; ++iter) {
        if (requiredModuleName == *iter) {
            return true;
        }
    }
    return false;
}

void SourceTextModule::PrintCircular(const CList<CString> &referenceList, Level level)
{
    LOG_ECMA(INFO) << "checkCircularImport begin ----------------------------------------";
    if (level == Level::ERROR) {
        for (auto iter : referenceList) {
            LOG_ECMA(ERROR) << "checkCircularImport record: " << iter;
        }
    } else {
        for (auto iter : referenceList) {
            LOG_ECMA(WARN) << "checkCircularImport record: " << iter;
        }
    }
    LOG_ECMA(INFO) << "checkCircularImport end ------------------------------------------";
}

// This function for module which execution doesn't through SourceTextModule::Evaluate
void SourceTextModule::RecordEvaluatedOrError(JSThread *thread, JSHandle<SourceTextModule> module)
{
    if (thread->HasPendingException()) {
        module->SetStatus(ModuleStatus::ERRORED);
        auto &options = const_cast<EcmaVM *>(thread->GetEcmaVM())->GetJSOptions();
        if (options.EnableModuleException()) {
            LOG_FULL(INFO) << "Error module: " << module->GetEcmaModuleRecordNameString();
        }
        return SetExceptionToModule(thread, module, thread->GetException());
    }
    module->SetStatus(ModuleStatus::EVALUATED);
}

void SourceTextModule::SetExceptionToModule(JSThread *thread, JSHandle<SourceTextModule> module,
                                            JSTaggedValue exception)
{
    if (!IsSharedModule(module)) {
        module->SetException(thread, exception);
        return;
    }
    JSHandle<JSTaggedValue> exceptionInfo(thread, exception);
    JSHandle<JSTaggedValue> ecmaErrMsg(thread, JSTaggedValue::Undefined());
    bool hasPendingException = thread->HasPendingException();
    if (hasPendingException) {
        thread->ClearException();
    }
    // process error message for share module
    if (exceptionInfo->IsJSError()) {
        CString msg = base::ErrorHelper::GetJSErrorInfo(thread, exceptionInfo,
                                                        base::ErrorHelper::JSErrorProps::MESSAGE);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        CString stack = base::ErrorHelper::GetJSErrorInfo(thread, exceptionInfo,
                                                          base::ErrorHelper::JSErrorProps::STACK);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        CString errMsg = "Error store in module " + module->GetEcmaModuleRecordNameString() + ":\n" + msg +
                         "\n" + stack;
        ecmaErrMsg = JSHandle<JSTaggedValue>::Cast(thread->GetEcmaVM()->GetFactory()->NewFromUtf8(errMsg));
    } else {
        ecmaErrMsg = JSHandle<JSTaggedValue>::Cast(JSTaggedValue::ToString(thread, exceptionInfo));
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
    if (hasPendingException) {
        thread->SetException(exceptionInfo.GetTaggedValue());
    }
    module->SetException(thread, ecmaErrMsg.GetTaggedValue());
    return;
}

// Add for dynamic import entry or other import ldvar
bool SourceTextModule::CheckAndThrowModuleError(JSThread *thread)
{
    if (GetStatus() == ModuleStatus::ERRORED) {
        LOG_FULL(ERROR) << "Error found in module:" << GetEcmaModuleRecordNameString();
        JSHandle<JSTaggedValue> exceptionInfo(thread, GetException());
        if (exceptionInfo->IsJSError()) {
            base::ErrorHelper::PrintJSErrorInfo(thread, exceptionInfo);
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, exceptionInfo.GetTaggedValue(), false);
        }
        JSHandle<EcmaString> message = JSTaggedValue::ToString(thread, exceptionInfo);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        CString str = ConvertToString(*message);
        LOG_NO_TAG(ERROR) << str;
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, message.GetTaggedValue(), false);
    }
    return true;
}

void SourceTextModule::HandleEvaluateException(JSThread *thread,
    const CVector<JSHandle<SourceTextModule>> &stack, JSHandle<JSTaggedValue> exception)
{
    // a. For each module m in stack, do
    auto &options = const_cast<EcmaVM *>(thread->GetEcmaVM())->GetJSOptions();
    if (options.EnableModuleException()) {
        JSTaggedValue::DesensitizedDump(exception);
    }
    for (auto &mm : stack) {
        // i. Assert: m.[[Status]] is "evaluating".
        ASSERT(mm->GetStatus() == ModuleStatus::EVALUATING);
        // ii. Set m.[[Status]] to "evaluated".
        mm->SetStatus(ModuleStatus::ERRORED);
        if (options.EnableModuleException()) {
            LOG_FULL(INFO) << "Error module: " << mm->GetEcmaModuleRecordNameString();
        }
        SetExceptionToModule(thread, mm, exception.GetTaggedValue());
    }
}

void SourceTextModule::HandleErrorStack(JSThread *thread, const CVector<JSHandle<SourceTextModule>> &errorStack)
{
    auto &options = const_cast<EcmaVM *>(thread->GetEcmaVM())->GetJSOptions();
    for (auto &mm : errorStack) {
        ASSERT(mm->GetStatus() == ModuleStatus::EVALUATING ||
                mm->GetStatus() == ModuleStatus::EVALUATED);
        // ii. Set m.[[Status]] to "error".
        mm->SetStatus(ModuleStatus::ERRORED);
        if (options.EnableModuleException()) {
            LOG_FULL(INFO) << "Error module: " << mm->GetEcmaModuleRecordNameString();
        }
    }
}

JSHandle<SourceTextModule> SourceTextModule::GetRequestedModule(JSThread *thread,
                                                                const JSHandle<TaggedArray> requestedModules,
                                                                uint32_t idx)
{
    JSHandle<JSTaggedValue> request(thread, requestedModules->Get(idx));
    if (request->IsSourceTextModule()) {
        // current is normal module: directly get require SourceTextModule.
        return JSHandle<SourceTextModule>::Cast(request);
    }
    // current is shared module: resolve or find request module by request string.
    ModuleManager *moduleManager = thread->GetModuleManager();
    CString requestStr = ModulePathHelper::Utf8ConvertToString(request.GetTaggedValue());
    return moduleManager->GetImportedModule(requestStr);
}
} // namespace panda::ecmascript
