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

#include "ecmascript/file_loader.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/jspandafile/js_pandafile.h"
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

JSTaggedValue ModuleManager::GetModuleValueInner(JSTaggedValue key)
{
    JSTaggedValue currentModule = GetCurrentModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetModuleValue(vm_->GetJSThread(), key, false);
}

JSTaggedValue ModuleManager::GetModuleValueInner(JSTaggedValue key, JSTaggedValue jsFunc)
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueInner currentModule failed";
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
        JSHandle<JSTaggedValue> cjsModuleName(thread, module->GetEcmaModuleFilename());
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
                                    << ConvertToString(EcmaString::Cast(referencing.GetTaggedObject()));
    JSTaggedValue result = dict->GetValue(entry);
    return JSHandle<SourceTextModule>(vm_->GetJSThread(), result);
}

bool ModuleManager::IsImportedModuleLoaded(JSTaggedValue referencing)
{
    int entry = NameDictionary::Cast(resolvedModules_.GetTaggedObject())->FindEntry(referencing);
    return (entry != -1);
}

JSHandle<SourceTextModule> ModuleManager::HostResolveImportedModuleWithMerge(const CString &moduleFileName,
                                                                             const CString &recordName)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<EcmaString> recordNameHandle = factory->NewFromUtf8(recordName);
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(recordNameHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<SourceTextModule>(thread, dict->GetValue(entry));
    }

    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, recordName.c_str());
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open jsPandaFile " << moduleFileName << " error";
        UNREACHABLE();
    }

    return ResolveModuleWithMerge(thread, jsPandaFile, recordName);
}

JSHandle<SourceTextModule> ModuleManager::HostResolveImportedModule(const CString &referencingModule)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<EcmaString> referencingHandle = factory->NewFromUtf8(referencingModule);
    CString moduleFileName = referencingModule;
    if (!vm_->GetResolvePathCallback()) {
        if (FileLoader::GetAbsolutePath(referencingModule, moduleFileName)) {
            referencingHandle = factory->NewFromUtf8(moduleFileName);
        } else {
            LOG_ECMA(ERROR) << "absolute " << referencingModule << " path error";
            UNREACHABLE();
        }
    }

    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<SourceTextModule>(thread, dict->GetValue(entry));
    }

    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(FATAL) << "open jsPandaFile " << moduleFileName << " error";
        UNREACHABLE();
    }

    return ResolveModule(thread, jsPandaFile);
}

JSHandle<SourceTextModule> ModuleManager::HostResolveImportedModule(const void *buffer, size_t size,
                                                                    const CString &filename)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<EcmaString> referencingHandle = factory->NewFromUtf8(filename);
    NameDictionary *dict = NameDictionary::Cast(resolvedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<SourceTextModule>(thread, dict->GetValue(entry));
    }

    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, filename,
                                                           JSPandaFile::ENTRY_MAIN_FUNCTION, buffer, size);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open jsPandaFile " << filename << " error";
        UNREACHABLE();
    }

    return ResolveModule(thread, jsPandaFile);
}

JSHandle<SourceTextModule> ModuleManager::ResolveModule(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    ObjectFactory *factory = vm_->GetFactory();
    CString moduleFileName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<JSTaggedValue> moduleRecord = thread->GlobalConstants()->GetHandledUndefined();
    if (jsPandaFile->IsCjs()) {
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, moduleFileName);
    } else if (jsPandaFile->IsModule()) {
        moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, moduleFileName, moduleFileName);
    } else {
        LOG_ECMA(ERROR) << "jsPandaFile: " << moduleFileName << " is not CjsModule or EcmaModule";
        UNREACHABLE();
    }

    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    JSHandle<JSTaggedValue> referencingHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(moduleFileName));
    resolvedModules_ =
        NameDictionary::Put(thread, dict, referencingHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
    return JSHandle<SourceTextModule>::Cast(moduleRecord);
}

JSHandle<SourceTextModule> ModuleManager::ResolveModuleWithMerge(
    JSThread *thread, const JSPandaFile *jsPandaFile, const CString &recordName)
{
    ObjectFactory *factory = vm_->GetFactory();
    CString moduleFileName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<JSTaggedValue> moduleRecord = thread->GlobalConstants()->GetHandledUndefined();
    if (jsPandaFile->IsCjs(recordName)) {
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, moduleFileName);
    } else if (jsPandaFile->IsModule(recordName)) {
        moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, recordName, moduleFileName);
    } else {
        LOG_ECMA(ERROR) << "jsPandaFile: " << moduleFileName << " is not CjsModule or EcmaModule";
        UNREACHABLE();
    }

    JSHandle<JSTaggedValue> recordNameHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(recordName));
    JSHandle<SourceTextModule>::Cast(moduleRecord)->SetEcmaModuleRecordName(thread, recordNameHandle);
    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    resolvedModules_ =
        NameDictionary::Put(thread, dict, recordNameHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
    return JSHandle<SourceTextModule>::Cast(moduleRecord);
}

void ModuleManager::ConcatFileName(std::string &dirPath, std::string &requestPath, std::string &fileName)
{
    JSThread *thread = vm_->GetJSThread();
    int suffixEnd = static_cast<int>(requestPath.find_last_of('.'));
    if (suffixEnd == -1) {
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
#if defined(PANDA_TARGET_WINDOWS)
    if (requestPath[1] == ':') { // absoluteFilePath
        fileName = requestPath.substr(0, suffixEnd) + ".abc";
    } else {
        int pos = static_cast<int>(dirPath.find_last_of('\\'));
        if (pos == -1) {
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
        fileName = dirPath.substr(0, pos + 1) + requestPath.substr(0, suffixEnd) + ".abc";
    }
#else
    if (requestPath.find("./") == 0) {
        requestPath = requestPath.substr(2); // 2 : delete './'
        suffixEnd -=2; // 2 : delete './'
    }
    if (requestPath[0] == '/') { // absoluteFilePath
        fileName = requestPath.substr(0, suffixEnd) + ".abc";
    } else {
        int pos = static_cast<int>(dirPath.find_last_of('/'));
        if (pos == -1) {
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
        fileName = dirPath.substr(0, pos + 1) + requestPath.substr(0, suffixEnd) + ".abc";
    }
#endif
}

JSHandle<SourceTextModule> ModuleManager::HostResolveImportedModule(std::string &baseFilename,
                                                                    std::string &moduleFilename)
{
    JSThread *thread = vm_->GetJSThread();
    bool mode = GetCurrentMode();
    std::string moduleFullname;
    if (!mode) {
        ResolvePathCallback resolvePathCallback = thread->GetEcmaVM()->GetResolvePathCallback();
        if (resolvePathCallback != nullptr) {
            moduleFullname = resolvePathCallback(baseFilename, moduleFilename);
            if (moduleFullname == "") {
                LOG_FULL(FATAL) << "dirPath: " << baseFilename << "\n" << " requestPath: " << moduleFilename << "\n"
                                << " moduleRequest callbackModuleName is hole failed";
                UNREACHABLE();
            }
            return HostResolveImportedModule(moduleFullname.c_str());
        }
        ConcatFileName(baseFilename, moduleFilename, moduleFullname);
        return HostResolveImportedModule(moduleFullname.c_str());
    } else {
        // mode == true buffer
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
        ResolveBufferCallback resolveBufferCallback = thread->GetEcmaVM()->GetResolveBufferCallback();
        if (resolveBufferCallback != nullptr) {
            std::vector<uint8_t> data = resolveBufferCallback(baseFilename, moduleFilename);
            size_t size = data.size();
            if (data.empty()) {
                LOG_FULL(FATAL) << " moduleRequest callbackModuleName " << moduleFullname << "is hole failed";
                UNREACHABLE();
            }
            ConcatFileName(baseFilename, moduleFilename, moduleFullname);
            return HostResolveImportedModule(data.data(),
                size, moduleFullname.c_str());
        }
#else
        ResolvePathCallback resolvePathCallback = thread->GetEcmaVM()->GetResolvePathCallback();
        std::string modulePath = moduleFilename;
#ifdef PANDA_TARGET_WINDOWS
        replace(modulePath.begin(), modulePath.end(), '/', '\\');
#endif
        if (resolvePathCallback != nullptr) {
            moduleFullname = resolvePathCallback(baseFilename, modulePath);
            if (moduleFullname == "") {
                LOG_FULL(FATAL) << "dirPath: " << baseFilename << "\n" << " requestPath: " << modulePath << "\n"
                                << " moduleRequest callbackModuleName is hole failed";
                UNREACHABLE();
            }
            return HostResolveImportedModule(moduleFullname.c_str());
        }
#endif
        return JSHandle<SourceTextModule>(thread, JSTaggedValue::Undefined());
    }
}

void ModuleManager::AddResolveImportedModule(const JSPandaFile *jsPandaFile, const CString &referencingModule)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> moduleRecord =
        ModuleDataExtractor::ParseModule(thread, jsPandaFile, referencingModule, referencingModule);
    JSHandle<JSTaggedValue> referencingHandle(factory->NewFromUtf8(referencingModule));
    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    resolvedModules_ =
        NameDictionary::Put(thread, dict, referencingHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
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

CString ModuleManager::ResolveModuleFileName(const CString &fileName)
{
    JSHandle<SourceTextModule> sourceTextModule = HostResolveImportedModule(fileName);
    return ConvertToString(sourceTextModule->GetEcmaModuleFilename());
}
} // namespace panda::ecmascript
