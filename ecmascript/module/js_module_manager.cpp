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
    JSHandle<JSTaggedValue> referencingHandle =
        JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(referencingModule));
    int entry =
        NameDictionary::Cast(resolvedModules_.GetTaggedObject())->FindEntry(referencingHandle.GetTaggedValue());
    LOG_ECMA_IF(entry == -1, FATAL) << "cannot get module: " << referencingModule;

    return JSHandle<SourceTextModule>(vm_->GetJSThread(),
                                      NameDictionary::Cast(resolvedModules_.GetTaggedObject())->GetValue(entry));
}

JSHandle<SourceTextModule> ModuleManager::HostGetImportedModule(JSTaggedValue referencingModule)
{
    int entry =
        NameDictionary::Cast(resolvedModules_.GetTaggedObject())->FindEntry(referencingModule);
    LOG_ECMA_IF(entry == -1, FATAL) << "cannot get module: " << ConvertToString(referencingModule);

    return JSHandle<SourceTextModule>(vm_->GetJSThread(),
                                      NameDictionary::Cast(resolvedModules_.GetTaggedObject())->GetValue(entry));
}

bool ModuleManager::resolveImportedModule(JSTaggedValue referencingModule)
{
    int entry =
        NameDictionary::Cast(resolvedModules_.GetTaggedObject())->FindEntry(referencingModule);
    return entry != -1;
}

JSHandle<SourceTextModule> ModuleManager::HostResolveImportedModule(const CString &referencingModule)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalConstants = thread->GlobalConstants();
    JSHandle<JSTaggedValue> referencingHandle =
        JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(referencingModule));
    CString moduleFileName = referencingModule;
    if (!vm_->GetResolvePathCallback()) {
        std::string absPath;
        std::string moduleName = CstringConvertToStdString(moduleFileName);
        if (FileLoader::GetAbsolutePath(moduleName, absPath)) {
            moduleFileName = ConvertToString(absPath);
            referencingHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(moduleFileName));
        } else {
            LOG_ECMA(ERROR) << "absolute " << referencingModule << " path error";
            UNREACHABLE();
        }
    }
    int entry =
        NameDictionary::Cast(resolvedModules_.GetTaggedObject())->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<SourceTextModule>(
            thread, NameDictionary::Cast(resolvedModules_.GetTaggedObject())->GetValue(entry));
    }

    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFileName, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open jsPandaFile " << moduleFileName << " error";
        UNREACHABLE();
    }
    JSHandle<JSTaggedValue> moduleRecord = globalConstants->GetHandledUndefined();
    if (jsPandaFile->IsCjs()) {
        moduleRecord = ModuleDataExtractor::ParseCjsModule(thread, moduleFileName);
    } else if (jsPandaFile->IsModule()) {
        moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, moduleFileName);
    } else {
        LOG_ECMA(ERROR) << "jsPandaFile: " << moduleFileName << " is not CjsModule or EcmaModule";
        UNREACHABLE();
    }

    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    resolvedModules_ =
        NameDictionary::Put(thread, dict, referencingHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
    return JSHandle<SourceTextModule>::Cast(moduleRecord);
}

void ModuleManager::AddResolveImportedModule(const JSPandaFile *jsPandaFile, const CString &referencingModule)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, referencingModule);
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
