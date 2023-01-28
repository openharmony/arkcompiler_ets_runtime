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
#include "ecmascript/module/js_module_manager.h"

#include "ecmascript/aot_file_manager.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_array.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/require/js_cjs_module.h"
#ifdef PANDA_TARGET_WINDOWS
#include <algorithm>
#endif

namespace panda::ecmascript {
using BuiltinsJson = builtins::BuiltinsJson;

ModuleManager::ModuleManager(EcmaVM *vm) : vm_(vm)
{
    resolvedModules_ = NameDictionary::Create(vm_->GetJSThread(), DEAULT_DICTIONART_CAPACITY).GetTaggedValue();
}

JSTaggedValue ModuleManager::GetCurrentModule()
{
    FrameHandler frameHandler(vm_->GetJSThread());
    JSTaggedValue currentFunc = frameHandler.GetFunction();
    return JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule();
}

JSTaggedValue ModuleManager::GetModuleValueInner(int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), index, false);
}

JSTaggedValue ModuleManager::GetModuleValueInner(int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), index, false);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule();
    return GetModuleValueOutterInternal(index, currentModule);
}

JSTaggedValue ModuleManager::GetModuleName(JSTaggedValue currentModule)
{
    SourceTextModule *module = SourceTextModule::Cast(currentModule.GetTaggedObject());
    JSTaggedValue recordName = module->GetEcmaModuleRecordName();
    if (recordName.IsUndefined()) {
        return module->GetEcmaModuleFilename();
    } else {
        return recordName;
    }
}

JSTaggedValue ModuleManager::GetModuleValueOutter(int32_t index, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetModuleValueOutterInternal(index, currentModule);
}

JSTaggedValue ModuleManager::GetModuleValueOutterInternal(int32_t index, JSTaggedValue currentModule)
{
    JSThread *thread = vm_->GetJSThread();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueOutter currentModule failed";
        UNREACHABLE();
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(currentModule.GetTaggedObject())->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return vm_->GetJSThread()->GlobalConstants()->GetUndefined();
    }
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnvironment.GetTaggedObject())->Get(index);
    ASSERT(resolvedBinding.IsResolvedIndexBinding());
    ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
    JSTaggedValue resolvedModule = binding->GetModule();
    ASSERT(resolvedModule.IsSourceTextModule());
    SourceTextModule *module = SourceTextModule::Cast(resolvedModule.GetTaggedObject());
    if (module->GetTypes() == ModuleTypes::CJSMODULE) {
        JSHandle<JSTaggedValue> cjsModuleName(thread, GetModuleName(JSTaggedValue(module)));
        JSHandle<JSTaggedValue> cjsExports = CjsModule::SearchFromModuleCache(thread, cjsModuleName);
        // if cjsModule is not JSObject, means cjs uses default exports.
        if (!cjsExports->IsJSObject()) {
            if (cjsExports->IsHole()) {
                ObjectFactory *factory = vm_->GetFactory();
                JSHandle<JSTaggedValue> currentModuleName(thread, SourceTextModule::Cast(
                    currentModule.GetTaggedObject())->GetEcmaModuleFilename());
                CString errorMsg = "currentModule" + ConvertToString(currentModuleName.GetTaggedValue()) +
                                   "find requireModule" + ConvertToString(cjsModuleName.GetTaggedValue()) + "failed";
                JSHandle<JSObject> syntaxError =
                    factory->GetJSError(base::ErrorType::SYNTAX_ERROR, errorMsg.c_str());
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), JSTaggedValue::Exception());
            }
            return cjsExports.GetTaggedValue();
        }
        int32_t idx = binding->GetIndex();
        if (idx == -1) {
            return cjsExports.GetTaggedValue();
        }
        JSObject *cjsObject = JSObject::Cast(cjsExports.GetTaggedValue());
        JSHClass *jsHclass = cjsObject->GetJSHClass();
        LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
        PropertyAttributes attr = layoutInfo->GetAttr(idx);
        JSTaggedValue value = cjsObject->GetProperty(jsHclass, attr);
        if (UNLIKELY(value.IsAccessor())) {
            return FastRuntimeStub::CallGetter(thread, JSTaggedValue(cjsObject), JSTaggedValue(cjsObject), value);
        }
        ASSERT(!value.IsAccessor());
        return value;
    }
    return SourceTextModule::Cast(resolvedModule.GetTaggedObject())->GetModuleValue(thread,
                                                                                    binding->GetIndex(), false);
}

void ModuleManager::StoreModuleValue(int32_t index, JSTaggedValue value)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, GetCurrentModule());
    StoreModuleValueInternal(currentModule, index, value);
}

void ModuleManager::StoreModuleValue(int32_t index, JSTaggedValue value, JSTaggedValue jsFunc)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule());
    StoreModuleValueInternal(currentModule, index, value);
}

void ModuleManager::StoreModuleValueInternal(JSHandle<SourceTextModule> &currentModule,
                                             int32_t index, JSTaggedValue value)
{
    if (currentModule.GetTaggedValue().IsUndefined()) {
        LOG_FULL(FATAL) << "StoreModuleValue currentModule failed";
        UNREACHABLE();
    }
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    currentModule->StoreModuleValue(thread, index, valueHandle);
}

JSTaggedValue ModuleManager::GetModuleValueInner(JSTaggedValue key)
{
    JSTaggedValue currentModule = GetCurrentModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
        UNREACHABLE();
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), key, false);
}

JSTaggedValue ModuleManager::GetModuleValueInner(JSTaggedValue key, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
        UNREACHABLE();
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), key, false);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(JSTaggedValue key)
{
    JSTaggedValue currentModule = GetCurrentModule();
    return GetModuleValueOutterInternal(key, currentModule);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(JSTaggedValue key, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetModuleValueOutterInternal(key, currentModule);
}

JSTaggedValue ModuleManager::GetModuleValueOutterInternal(JSTaggedValue key, JSTaggedValue currentModule)
{
    JSThread *thread = vm_->GetJSThread();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueOutter currentModule failed";
        UNREACHABLE();
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(currentModule.GetTaggedObject())->GetEnvironment();
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
    if (module->GetTypes() == ModuleTypes::CJSMODULE) {
        JSHandle<JSTaggedValue> cjsModuleName(thread, GetModuleName(JSTaggedValue(module)));
        return CjsModule::SearchFromModuleCache(thread, cjsModuleName).GetTaggedValue();
    }
    return SourceTextModule::Cast(resolvedModule.GetTaggedObject())->GetModuleValue(thread,
                                                                                    binding->GetBindingName(), false);
}

void ModuleManager::StoreModuleValue(JSTaggedValue key, JSTaggedValue value)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, GetCurrentModule());
    StoreModuleValueInternal(currentModule, key, value);
}

void ModuleManager::StoreModuleValue(JSTaggedValue key, JSTaggedValue value, JSTaggedValue jsFunc)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule());
    StoreModuleValueInternal(currentModule, key, value);
}

void ModuleManager::StoreModuleValueInternal(JSHandle<SourceTextModule> &currentModule,
                                             JSTaggedValue key, JSTaggedValue value)
{
    if (currentModule.GetTaggedValue().IsUndefined()) {
        LOG_FULL(FATAL) << "StoreModuleValue currentModule failed";
        UNREACHABLE();
    }
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    currentModule->StoreModuleValue(thread, keyHandle, valueHandle);
}

JSHandle<SourceTextModule> ModuleManager::HostGetImportedModule(const CString &referencingModule)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<EcmaString> referencingHandle = factory->NewFromUtf8(referencingModule);
    return HostGetImportedModule(referencingHandle.GetTaggedValue());
}

JSHandle<SourceTextModule> ModuleManager::HostGetImportedModule(JSTaggedValue referencing)
{
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencing);
    LOG_ECMA_IF(entry == -1, FATAL) << "Can not get module: "
                                    << ConvertToString(referencing);
    JSTaggedValue result = dict->GetValue(entry);
    return JSHandle<SourceTextModule>(vm_->GetJSThread(), result);
}

bool ModuleManager::IsImportedModuleLoaded(JSTaggedValue referencing)
{
    int entry = NameDictionary::Cast(resolvedModules_.GetTaggedObject())->FindEntry(referencing);
    return (entry != -1);
}

JSHandle<JSTaggedValue> ModuleManager::HostResolveImportedModuleWithMerge(const CString &moduleFileName,
                                                                          const CString &recordName)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<EcmaString> recordNameHandle = factory->NewFromUtf8(recordName);
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(recordNameHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<JSTaggedValue>(thread, dict->GetValue(entry));
    }
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, recordName, false);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "Try to load record " << recordName << " in abc : " << moduleFileName;
        CString msg = "Faild to load file '" + recordName + "', please check the request path.";

        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }

    JSHandle<JSTaggedValue> moduleRecord = ResolveModuleWithMerge(thread, jsPandaFile, recordName);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    JSHandle<NameDictionary> handleDict(thread, resolvedModules_);
    resolvedModules_ = NameDictionary::Put(thread, handleDict, JSHandle<JSTaggedValue>(recordNameHandle),
                                           JSHandle<JSTaggedValue>(moduleRecord), PropertyAttributes::Default())
                                           .GetTaggedValue();

    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleManager::HostResolveImportedModule(const CString &referencingModule)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<EcmaString> referencingHandle = factory->NewFromUtf8(referencingModule);
    CString moduleFileName = referencingModule;
    if (vm_->IsBundlePack()) {
        if (AOTFileManager::GetAbsolutePath(referencingModule, moduleFileName)) {
            referencingHandle = factory->NewFromUtf8(moduleFileName);
        } else {
            CString msg = "Parse absolute " + referencingModule + " path failed";
            THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
        }
    }

    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<JSTaggedValue>(thread, dict->GetValue(entry));
    }

    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "Try to load abc : " << moduleFileName;
        CString msg = "Faild to load file '" + moduleFileName + "', please check the request path.";

        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }

    return ResolveModule(thread, jsPandaFile);
}

JSHandle<JSTaggedValue> ModuleManager::HostResolveImportedModule(const void *buffer, size_t size,
                                                                 const CString &filename)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<EcmaString> referencingHandle = factory->NewFromUtf8(filename);
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<JSTaggedValue>(thread, dict->GetValue(entry));
    }

    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, filename,
                                                           JSPandaFile::ENTRY_MAIN_FUNCTION, buffer, size);
    if (jsPandaFile == nullptr) {
        CString msg = "Faild to load file '" + filename + "', please check the request path.";
        THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, JSTaggedValue, msg.c_str());
    }

    return ResolveModule(thread, jsPandaFile);
}

JSHandle<JSTaggedValue> ModuleManager::ResolveModule(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    ObjectFactory *factory = vm_->GetFactory();
    CString moduleFileName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<JSTaggedValue> moduleRecord = thread->GlobalConstants()->GetHandledUndefined();
    if (jsPandaFile->IsModule(thread)) {
        moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, moduleFileName, moduleFileName);
    } else if (jsPandaFile->IsJson(thread)) {
        moduleRecord = ModuleDataExtractor::ParseJsonModule(thread, jsPandaFile, moduleFileName);
    } else {
        ASSERT(jsPandaFile->IsCjs(thread));
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, jsPandaFile);
    }

    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    JSHandle<JSTaggedValue> referencingHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(moduleFileName));
    resolvedModules_ =
        NameDictionary::Put(thread, dict, referencingHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
    return moduleRecord;
}

JSHandle<JSTaggedValue> ModuleManager::ResolveModuleWithMerge(
    JSThread *thread, const JSPandaFile *jsPandaFile, const CString &recordName)
{
    ObjectFactory *factory = vm_->GetFactory();
    CString moduleFileName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<JSTaggedValue> moduleRecord = thread->GlobalConstants()->GetHandledUndefined();
    if (jsPandaFile->IsModule(thread, recordName)) {
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, recordName, moduleFileName);
    } else if (jsPandaFile->IsJson(thread, recordName)) {
        moduleRecord = ModuleDataExtractor::ParseJsonModule(thread, jsPandaFile, moduleFileName, recordName);
    } else {
        ASSERT(jsPandaFile->IsCjs(thread, recordName));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, jsPandaFile);
    }

    JSHandle<JSTaggedValue> recordNameHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(recordName));
    JSHandle<SourceTextModule>::Cast(moduleRecord)->SetEcmaModuleRecordName(thread, recordNameHandle);
    return moduleRecord;
}

void ModuleManager::AddResolveImportedModule(const JSPandaFile *jsPandaFile, const CString &referencingModule)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> moduleRecord =
        ModuleDataExtractor::ParseModule(thread, jsPandaFile, referencingModule, referencingModule);
    AddResolveImportedModule(referencingModule, moduleRecord);
}

void ModuleManager::AddResolveImportedModule(const CString &referencingModule, JSHandle<JSTaggedValue> moduleRecord)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> referencingHandle(factory->NewFromUtf8(referencingModule));
    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    resolvedModules_ =
        NameDictionary::Put(thread, dict, referencingHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
}

JSTaggedValue ModuleManager::GetModuleNamespace(int32_t index)
{
    JSTaggedValue currentModule = GetCurrentModule();
    return GetModuleNamespaceInternal(index, currentModule);
}

JSTaggedValue ModuleManager::GetModuleNamespace(int32_t index, JSTaggedValue currentFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule();
    return GetModuleNamespaceInternal(index, currentModule);
}

JSTaggedValue ModuleManager::GetModuleNamespaceInternal(int32_t index, JSTaggedValue currentModule)
{
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleNamespace currentModule failed";
        UNREACHABLE();
    }
    JSThread *thread = vm_->GetJSThread();
    SourceTextModule *module = SourceTextModule::Cast(currentModule.GetTaggedObject());
    JSTaggedValue requestedModule = module->GetRequestedModules();
    JSTaggedValue moduleName = TaggedArray::Cast(requestedModule.GetTaggedObject())->Get(index);
    JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
    JSHandle<JSTaggedValue> requiredModule;
    if (moduleRecordName.IsUndefined()) {
        requiredModule = SourceTextModule::HostResolveImportedModule(thread,
            JSHandle<SourceTextModule>(thread, module), JSHandle<JSTaggedValue>(thread, moduleName));
    } else {
        ASSERT(moduleRecordName.IsString());
        requiredModule = SourceTextModule::HostResolveImportedModuleWithMerge(thread,
            JSHandle<SourceTextModule>(thread, module), JSHandle<JSTaggedValue>(thread, moduleName));
    }

    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    // if requiredModuleST is CommonJS
    JSHandle<SourceTextModule> requiredModuleST = JSHandle<SourceTextModule>::Cast(requiredModule);
    if (requiredModuleST->GetTypes() == ModuleTypes::CJSMODULE) {
        JSHandle<JSTaggedValue> cjsModuleName(thread, GetModuleName(requiredModuleST.GetTaggedValue()));
        return CjsModule::SearchFromModuleCache(thread, cjsModuleName).GetTaggedValue();
    }
    // if requiredModuleST is ESM
    JSHandle<JSTaggedValue> moduleNamespace = SourceTextModule::GetModuleNamespace(thread, requiredModuleST);
    ASSERT(moduleNamespace->IsModuleNamespace());
    return moduleNamespace.GetTaggedValue();
}

JSTaggedValue ModuleManager::GetModuleNamespace(JSTaggedValue localName)
{
    JSTaggedValue currentModule = GetCurrentModule();
    return GetModuleNamespaceInternal(localName, currentModule);
}

JSTaggedValue ModuleManager::GetModuleNamespace(JSTaggedValue localName, JSTaggedValue currentFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule();
    return GetModuleNamespaceInternal(localName, currentModule);
}

JSTaggedValue ModuleManager::GetModuleNamespaceInternal(JSTaggedValue localName, JSTaggedValue currentModule)
{
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleNamespace currentModule failed";
        UNREACHABLE();
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(currentModule.GetTaggedObject())->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return vm_->GetJSThread()->GlobalConstants()->GetUndefined();
    }
    int entry = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->FindEntry(localName);
    if (entry == -1) {
        return vm_->GetJSThread()->GlobalConstants()->GetUndefined();
    }
    JSTaggedValue moduleNamespace = NameDictionary::Cast(moduleEnvironment.GetTaggedObject())->GetValue(entry);
    ASSERT(moduleNamespace.IsModuleNamespace());
    return moduleNamespace;
}

void ModuleManager::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&resolvedModules_)));
}

CString ModuleManager::ConcatFileNameWithMerge(const JSPandaFile *jsPandaFile, CString &baseFilename,
                                               CString moduleRecordName, CString moduleRequestName)
{
    CString entryPoint;
    size_t pos = 0;
    size_t typePos = CString::npos;
    if (moduleRequestName.find("@bundle:") != CString::npos) {
        moduleRequestName = moduleRequestName.substr(JSPandaFile::MODULE_OR_BUNDLE_PREFIX_LEN);
        pos = moduleRequestName.find('/');
        CString bundleName = moduleRequestName.substr(0, pos);
        size_t bundleNameLen = bundleName.length();
        entryPoint = moduleRequestName;
        if (jsPandaFile->IsNewRecord()) {
            bool isDifferentBundle = (moduleRecordName.length() > bundleNameLen) &&
            (moduleRecordName.compare(0, bundleNameLen, bundleName) != 0);
            pos = moduleRequestName.find('/', bundleNameLen + 1);
            if (isDifferentBundle && CString::npos != pos) {
                CString moduleName = moduleRequestName.substr(bundleNameLen + 1, pos - bundleNameLen - 1);
                baseFilename = JSPandaFile::BUNDLE_INSTALL_PATH + moduleRequestName.substr(0, pos) + '/' + moduleName +
                            JSPandaFile::MERGE_ABC_ETS_MODULES;
            }
        } else {
            JSPandaFile::CroppingRecord(entryPoint);
        }
    } else if (moduleRequestName.find("@module:") != CString::npos) {
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
        moduleRequestName = moduleRequestName.substr(JSPandaFile::MODULE_OR_BUNDLE_PREFIX_LEN);
        pos = moduleRequestName.find('/');
        ASSERT(pos != CString::npos);
        baseFilename =
            JSPandaFile::BUNDLE_INSTALL_PATH + moduleRequestName.substr(0, pos) + JSPandaFile::MERGE_ABC_ETS_MODULES;
        pos = moduleRecordName.find('/');
        entryPoint = moduleRecordName.substr(0, pos + 1) + moduleRequestName;
#else
        entryPoint = JSPandaFile::PREVIEW_OF_ACROSS_HAP_FLAG;
        LOG_NO_TAG(ERROR) << "[ArkRuntime Log] Importing shared package is not supported in the Previewer.";
#endif
    } else if (IsImportedPath(moduleRequestName, typePos)) {
        if (typePos != CString::npos) {
            moduleRequestName = moduleRequestName.substr(0, typePos);
        }
        pos = moduleRequestName.find("./");
        if (pos == 0) {
            moduleRequestName = moduleRequestName.substr(2); // 2 means jump "./"
        }
        pos = moduleRecordName.rfind('/');
        if (pos != CString::npos) {
            entryPoint = moduleRecordName.substr(0, pos + 1) + moduleRequestName;
        } else {
            entryPoint = moduleRequestName;
        }
        entryPoint = JSPandaFileExecutor::NormalizePath(entryPoint);
        if (!jsPandaFile->HasRecord(entryPoint)) {
            entryPoint += "/index";
        }
        if (!jsPandaFile->HasRecord(entryPoint)) {
            pos = baseFilename.rfind('/');
            if (pos != CString::npos) {
                baseFilename = baseFilename.substr(0, pos + 1) + moduleRequestName + ".abc";
            } else {
                baseFilename = moduleRequestName + ".abc";
            }
            pos = moduleRequestName.rfind('/');
            if (pos != CString::npos) {
                entryPoint = moduleRequestName.substr(pos + 1);
            } else {
                entryPoint = moduleRequestName;
            }
        }
    } else {
        pos = moduleRecordName.find(JSPandaFile::NODE_MODULES);
        CString key = "";
        if (pos != CString::npos) {
            auto info = const_cast<JSPandaFile *>(jsPandaFile)->FindRecordInfo(moduleRecordName);
            CString PackageName = info.npmPackageName;
            while ((pos = PackageName.rfind(JSPandaFile::NODE_MODULES)) != CString::npos) {
                key = PackageName + "/" + JSPandaFile::NODE_MODULES + moduleRequestName;
                AddIndexToEntryPoint(jsPandaFile, entryPoint, key);
                if (entryPoint.empty()) {
                    break;
                }
                PackageName = PackageName.substr(0, pos > 0 ? pos - 1 : 0);
            }
        }

        if (entryPoint.empty()) {
            key = JSPandaFile::NODE_MODULES_ZERO + moduleRequestName;
            AddIndexToEntryPoint(jsPandaFile, entryPoint, key);
        }

        if (entryPoint.empty()) {
            key = JSPandaFile::NODE_MODULES_ONE + moduleRequestName;
            AddIndexToEntryPoint(jsPandaFile, entryPoint, key);
        }

        if (entryPoint.empty()) {
            LOG_ECMA(ERROR) << "find entryPoint failed\n"
                            << "moduleRequestName : " << moduleRequestName << "\n"
                            << "moduleRecordName : " << moduleRecordName << "\n";
        }
    }
    return entryPoint;
}

bool ModuleManager::IsImportedPath(const CString &moduleRequestName, size_t &typePos)
{
    if (moduleRequestName.rfind(".js") != CString::npos) {
        typePos = moduleRequestName.rfind(".js");
        return true;
    } else if (moduleRequestName.rfind(".ts") != CString::npos) {
        typePos = moduleRequestName.rfind(".ts");
        return true;
    } else if (moduleRequestName.rfind(".ets") != CString::npos) {
        typePos = moduleRequestName.rfind(".ets");
        return true;
    }
    return moduleRequestName.find("./") == 0 || moduleRequestName.find("../") == 0;
}

void ModuleManager::AddIndexToEntryPoint(const JSPandaFile *jsPandaFile, CString &entryPoint, CString &key)
{
    entryPoint = jsPandaFile->FindNpmEntryPoint(key);
    if (entryPoint.empty()) {
        key += "/index";
        entryPoint = jsPandaFile->FindNpmEntryPoint(key);
    }
}

CString ModuleManager::GetRecordName(JSTaggedValue module)
{
    CString entry = "";
    if (module.IsString()) {
        entry = ConvertToString(module);
    }
    if (module.IsSourceTextModule()) {
        SourceTextModule *sourceTextModule = SourceTextModule::Cast(module.GetTaggedObject());
        if (sourceTextModule->GetEcmaModuleRecordName().IsString()) {
            entry = ConvertToString(sourceTextModule->GetEcmaModuleRecordName());
        }
    }
    return entry;
}

int ModuleManager::GetExportObjectIndex(EcmaVM *vm, JSHandle<SourceTextModule> ecmaModule,
                                        const std::string &key)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<TaggedArray> localExportEntries(thread, ecmaModule->GetLocalExportEntries());
    size_t exportEntriesLen = localExportEntries->GetLength();
    // 0: There's only one export value "default"
    int index = 0;
    JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    if (exportEntriesLen > 1) { // 1:  The number of export objects exceeds 1
        for (size_t idx = 0; idx < exportEntriesLen; idx++) {
            ee.Update(localExportEntries->Get(idx));
            if (EcmaStringAccessor(ee->GetExportName()).ToStdString() == key) {
                ASSERT(idx <= static_cast<size_t>(INT_MAX));
                index = static_cast<int>(idx);
                break;
            }
        }
    }
    return index;
}

JSTaggedValue ModuleManager::JsonParse(JSThread *thread, const JSPandaFile *jsPandaFile, CString entryPoint)
{
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(
            thread, undefined, undefined, undefined, 1); // 1 : argument numbers
    CString value = jsPandaFile->GetJsonStringId(thread, entryPoint);
    JSHandle<JSTaggedValue> arg0(thread->GetEcmaVM()->GetFactory()->NewFromASCII(value));
    info->SetCallArg(arg0.GetTaggedValue());
    return BuiltinsJson::Parse(info);
}
} // namespace panda::ecmascript
