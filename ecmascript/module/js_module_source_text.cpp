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
#include "ecmascript/base/string_helper.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
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
        if (entryValue.IsStarExportEntry()) {
            // handle one starExportEntry
            ee.Update(entryValue);
            moduleRequest.Update(ee->GetModuleRequest());
            SetExportName(thread, moduleRequest, module, exportedNames, newExportStarSet);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exportedNames);
        } else {
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
    }
    return exportedNames;
}

// new way with module
JSHandle<SourceTextModule> SourceTextModule::HostResolveImportedModuleWithMerge(
    JSThread *thread, const JSHandle<SourceTextModule> &module, const JSHandle<JSTaggedValue> &moduleRequest,
    CVector<CString> &npmStack, bool &npm)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto moduleManager = thread->GetEcmaVM()->GetModuleManager();
    if (moduleManager->IsImportedModuleLoaded(moduleRequest.GetTaggedValue())) {
        return moduleManager->HostGetImportedModule(moduleRequest.GetTaggedValue());
    }
    ASSERT(module->GetEcmaModuleFilename().IsHeapObject());
    CString baseFilename =
        ConvertToString(EcmaString::Cast(module->GetEcmaModuleFilename().GetTaggedObject()));
    ASSERT(module->GetEcmaModuleRecordName().IsHeapObject());
    CString moduleRecordName =
        ConvertToString(EcmaString::Cast(module->GetEcmaModuleRecordName().GetTaggedObject()));

    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFilename, moduleRecordName);

    CString moduleRequestName = ConvertToString(EcmaString::Cast(moduleRequest->GetTaggedObject()));
    CString entryPoint;
    size_t pos = 0;
    if (moduleRequestName.find("@bundle:") != CString::npos) {
        pos = moduleRequestName.find('/');
        pos = moduleRequestName.find('/', pos + 1);
        ASSERT(pos != CString::npos);
        entryPoint = moduleRequestName.substr(pos + 1);
    } else if (moduleRequestName.rfind(".js") != CString::npos || moduleRequestName.find("./") == 0 ||
               moduleRequestName.find("../") == 0) {
        pos = moduleRequestName.rfind(".js");
        if (pos != CString::npos) {
            moduleRequestName = moduleRequestName.substr(0, pos);
        }
        pos = moduleRequestName.find("./");
        if (pos == 0) {
            moduleRequestName = moduleRequestName.substr(2); // jump "./"
        }
        size_t left = 0;
        while ((pos = moduleRequestName.find("../", left)) != CString::npos) {
            size_t index = moduleRecordName.rfind('/');
            ASSERT(index != CString::npos);
            moduleRecordName = moduleRecordName.substr(0, index);
            left = pos + 3; // 3 : means jump current "../"
        }
        moduleRequestName = moduleRequestName.substr(left);
        pos = moduleRecordName.rfind('/');
        if (pos != CString::npos) {
            entryPoint = moduleRecordName.substr(0, pos + 1) + moduleRequestName;
        } else {
            entryPoint = moduleRequestName;
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
        npm = true;
        pos = moduleRecordName.find(JSPandaFile::NODE_MODULES);
        CString key = "";
        if (pos != CString::npos) {
            key = npmStack.back() + "/" + JSPandaFile::NODE_MODULES + "/" + moduleRequestName;
            entryPoint = jsPandaFile->FindrecordName(key);
        }

        if (entryPoint.empty()) {
            key = JSPandaFile::NODE_MODULES_ZERO + moduleRequestName;
            entryPoint = jsPandaFile->FindrecordName(key);
        }

        if (entryPoint.empty()) {
            key = JSPandaFile::NODE_MODULES_ONE + moduleRequestName;
            entryPoint = jsPandaFile->FindrecordName(key);
        }
        npmStack.emplace_back(key);
        ASSERT(!entryPoint.empty());
    }
    return moduleManager->HostResolveImportedModuleWithMerge(baseFilename, entryPoint);
}

// old way with bundle
JSHandle<SourceTextModule> SourceTextModule::HostResolveImportedModule(JSThread *thread,
                                                                       const JSHandle<SourceTextModule> &module,
                                                                       const JSHandle<JSTaggedValue> &moduleRequest)
{
    auto moduleManage = thread->GetEcmaVM()->GetModuleManager();
    if (moduleManage->IsImportedModuleLoaded(moduleRequest.GetTaggedValue())) {
        return moduleManage->HostGetImportedModule(moduleRequest.GetTaggedValue());
    }
    std::string baseFilename = base::StringHelper::ToStdString(
        EcmaString::Cast(module->GetEcmaModuleFilename().GetTaggedObject()));
    std::string moduleFilename = base::StringHelper::ToStdString(
        EcmaString::Cast(moduleRequest->GetTaggedObject()));
    return thread->GetEcmaVM()->GetModuleManager()->HostResolveImportedModule(baseFilename, moduleFilename);
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveExport(JSThread *thread, const JSHandle<SourceTextModule> &module,
    const JSHandle<JSTaggedValue> &exportName,
    CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> &resolveSet)
{
    // 1. Let module be this Source Text Module Record.
    // 2. For each Record { [[Module]], [[ExportName]] } r in resolveSet, do
    auto globalConstants = thread->GlobalConstants();
    for (auto rr : resolveSet) {
        // a. If module and r.[[Module]] are the same Module Record and
        //    SameValue(exportName, r.[[ExportName]]) is true, then
        if (JSTaggedValue::SameValue(rr.first.GetTaggedValue(), module.GetTaggedValue()) &&
            JSTaggedValue::SameValue(rr.second, exportName)) {
            // i. Assert: This is a circular import request.
            // ii. Return null.
            return globalConstants->GetHandledNull();
        }
    }
    // 3. Append the Record { [[Module]]: module, [[ExportName]]: exportName } to resolveSet.
    resolveSet.emplace_back(std::make_pair(module, exportName));
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
                                                                   exportName, module, resolveSet);
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
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    if (starExportEntriesTv.IsStarExportEntry()) {
        ee.Update(starExportEntriesTv);
        moduleRequest.Update(ee->GetModuleRequest());
        JSHandle<JSTaggedValue> result = GetStarResolution(thread, exportName, moduleRequest,
                                                           module, starResolution, resolveSet);
        if (result->IsString() || result->IsException() || result->IsNull()) {
            return result;
        }
    } else {
        JSHandle<TaggedArray> starExportEntries(thread, starExportEntriesTv);
        size_t starExportEntriesLen = starExportEntries->GetLength();
        for (size_t idx = 0; idx < starExportEntriesLen; idx++) {
            ee.Update(starExportEntries->Get(idx));
            moduleRequest.Update(ee->GetModuleRequest());
            JSHandle<JSTaggedValue> result = GetStarResolution(thread, exportName, moduleRequest,
                                                               module, starResolution, resolveSet);
            if (result->IsString() || result->IsException()) {
                return result;
            }
        }
    }
    // 9. Return starResolution.
    return starResolution;
}

int SourceTextModule::Instantiate(JSThread *thread, const JSHandle<SourceTextModule> &module)
{
    // 1. Let module be this Source Text Module Record.
    // 2. Assert: module.[[Status]] is not "instantiating" or "evaluating".
    ASSERT(module->GetStatus() != ModuleStatus::INSTANTIATING && module->GetStatus() != ModuleStatus::EVALUATING);
    // 3. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    CVector<CString> npmStack;
    // 4. Let result be InnerModuleInstantiation(module, stack, 0).
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    int result = SourceTextModule::InnerModuleInstantiation(thread, moduleRecord, stack, 0, npmStack, false);
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
                                               CVector<JSHandle<SourceTextModule>> &stack, int index,
                                               CVector<CString> &npmStack, bool npm)
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
            bool isNpm = false;
            if (moduleRecordName.IsUndefined()) {
                requiredModule.Update(SourceTextModule::HostResolveImportedModule(thread, module, required));
                requestedModules->Set(thread, idx, requiredModule->GetEcmaModuleFilename());
            } else {
                ASSERT(moduleRecordName.IsString());
                requiredModule.Update(
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, required, npmStack, isNpm));
                requestedModules->Set(thread, idx, requiredModule->GetEcmaModuleRecordName());
            }

            // b. Set index to ? InnerModuleInstantiation(requiredModule, stack, index).
            JSHandle<ModuleRecord> requiredModuleRecord = JSHandle<ModuleRecord>::Cast(requiredModule);
            index = 
                SourceTextModule::InnerModuleInstantiation(thread, requiredModuleRecord, stack, index, npmStack, isNpm);
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

    // 10. Perform ? ModuleDeclarationEnvironmentSetup(module).
    SourceTextModule::ModuleDeclarationEnvironmentSetup(thread, module, npmStack);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
    if (npm && !npmStack.empty()) {
        npmStack.pop_back();
    }
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
                                                         const JSHandle<SourceTextModule> &module,
                                                         CVector<CString> &npmStack)
{
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
        bool isNpm = false;
        if (moduleRecordName.IsUndefined()) {
            importedModule.Update(SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest));
        } else {
            ASSERT(moduleRecordName.IsString());
            importedModule.Update(
                SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest, npmStack, isNpm));
        }
        if (isNpm) {
            npmStack.pop_back();
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
            CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
            JSHandle<JSTaggedValue> resolution =
                SourceTextModule::ResolveExport(thread, importedModule, importName, resolveSet);
            // ii. If resolution is null or "ambiguous", throw a SyntaxError exception.
            if (resolution->IsNull() || resolution->IsString()) {
                THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, "");
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
    JSMutableHandle<JSTaggedValue> moduleNamespace(thread, module->GetNamespace());
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
            CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
            JSHandle<JSTaggedValue> nameHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromStdString(name));
            JSHandle<JSTaggedValue> resolution =
                SourceTextModule::ResolveExport(thread, module, nameHandle, resolveSet);
            // ii. If resolution is a ResolvedBinding Record, append name to unambiguousNames.
            if (resolution->IsResolvedBinding()) {
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
                               const void *buffer, size_t size)
{
    // 1. Let module be this Source Text Module Record.
    // 2. Assert: module.[[Status]] is "instantiated" or "evaluated".
    [[maybe_unused]] ModuleStatus status = module->GetStatus();
    ASSERT((status == ModuleStatus::INSTANTIATED || status == ModuleStatus::EVALUATED));
    // 3. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    // 4. Let result be InnerModuleEvaluation(module, stack, 0)
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    int result = SourceTextModule::InnerModuleEvaluation(thread, moduleRecord, stack, 0, buffer, size);
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

int SourceTextModule::InnerModuleEvaluation(JSThread *thread, const JSHandle<ModuleRecord> &moduleRecord,
                                            CVector<JSHandle<SourceTextModule>> &stack, int index,
                                            const void *buffer, size_t size)
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
        SourceTextModule::ModuleExecution(thread, module, buffer, size);
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
        auto moduleManager = thread->GetEcmaVM()->GetModuleManager();
        for (size_t idx = 0; idx < requestedModulesLen; idx++) {
            required.Update(requestedModules->Get(idx));
            // a. Let requiredModule be ! HostResolveImportedModule(module, required).
            JSHandle<SourceTextModule> requiredModule =
                moduleManager->HostGetImportedModule(required.GetTaggedValue());
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
        }
    }

    // 11. Perform ? ModuleExecution(module).
    SourceTextModule::ModuleExecution(thread, module, buffer, size);
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

void SourceTextModule::ModuleExecution(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                       const void *buffer, size_t size)
{
    JSTaggedValue moduleFileName = module->GetEcmaModuleFilename();
    ASSERT(moduleFileName.IsString());
    CString moduleFilenameStr = ConvertToString(EcmaString::Cast(moduleFileName.GetTaggedObject()));

    std::string entryPoint;
    JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
    if (moduleRecordName.IsUndefined()) {
        entryPoint = JSPandaFile::ENTRY_FUNCTION_NAME;
    } else {
        ASSERT(moduleRecordName.IsString());
        entryPoint = ConvertToString(moduleRecordName);
    }

    const JSPandaFile *jsPandaFile = nullptr;
    if (buffer != nullptr) {
        jsPandaFile =
            JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFilenameStr, entryPoint, buffer, size);
    } else {
        jsPandaFile =
            JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFilenameStr, entryPoint);
    }

    if (jsPandaFile == nullptr) {
        LOG_ECMA(FATAL) << "open jsPandaFile " << moduleFilenameStr << " error";
        UNREACHABLE();
    }
    JSPandaFileExecutor::Execute(thread, jsPandaFile, entryPoint);
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
        if (len == 1) {
            module->SetLocalExportEntries(thread, exportEntry);
            return;
        }
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
        if (len == 1) {
            module->SetIndirectExportEntries(thread, exportEntry);
            return;
        }
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
        if (len == 1) {
            module->SetStarExportEntries(thread, exportEntry);
            return;
        }
        JSHandle<TaggedArray> array = factory->NewTaggedArray(len);
        array->Set(thread, idx, exportEntry.GetTaggedValue());
        module->SetStarExportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, starExportEntries);
        entries->Set(thread, idx, exportEntry.GetTaggedValue());
    }
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

    if (GetModes() == ModuleModes::ARRAYMODE) {
        JSTaggedValue tmp = GetModuleValueFromArray(JSTaggedValue(this), key);
        return tmp;
    }

    NameDictionary *dict = NameDictionary::Cast(dictionary.GetTaggedObject());
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        return dict->GetValue(entry);
    }

    JSTaggedValue exportEntriesTv = GetLocalExportEntries();
    if (!exportEntriesTv.IsUndefined()) {
        JSTaggedValue resolution = FindByExport(exportEntriesTv, key, dictionary);
        if (!resolution.IsHole()) {
            return resolution;
        }
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue SourceTextModule::GetModuleValueFromArray(const JSTaggedValue &sourceTextmodule, JSTaggedValue &key)
{
    DISALLOW_GARBAGE_COLLECTION;
    SourceTextModule *module = SourceTextModule::Cast(sourceTextmodule.GetTaggedObject());
    JSTaggedValue dictionary = module->GetNameDictionary();
    int entry = FindEntryFromArray(dictionary, key);
    if (entry != -1) {
        return GetValueFromArray(dictionary, entry);
    }

    JSTaggedValue exportEntriesTv = module->GetLocalExportEntries();
    if (!exportEntriesTv.IsUndefined()) {
        JSTaggedValue resolution = FindArrayByExport(exportEntriesTv, key, dictionary);
        if (!resolution.IsHole()) {
            return resolution;
        }
    }
    return JSTaggedValue::Hole();
}

void SourceTextModule::StoreModuleValue(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                        const JSHandle<JSTaggedValue> &value)
{
    JSHandle<SourceTextModule> module(thread, this);
    if (module->GetModes() == ModuleModes::ARRAYMODE) {
        return StoreModuleValueFromArray(thread, key, value);
    }
    JSMutableHandle<JSTaggedValue> data(thread, module->GetNameDictionary());
    if (data->IsUndefined()) {
        data.Update(NameDictionary::Create(thread, DEFAULT_DICTIONART_CAPACITY));
    }

    JSMutableHandle<NameDictionary> dataDict(data);
    JSHandle<JSTaggedValue> localExportEntriesTv(thread, module->GetLocalExportEntries());
    if (!localExportEntriesTv->IsUndefined()) {
        StoreByLocalExport(thread, localExportEntriesTv, value, key, dataDict);
    }

    module->SetNameDictionary(thread, dataDict);
}

void SourceTextModule::StoreModuleValueFromArray(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                                 const JSHandle<JSTaggedValue> &value)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module(thread, this);

    JSMutableHandle<JSTaggedValue> data(thread, module->GetNameDictionary());
    if (data->IsUndefined()) {
        data.Update(factory->EmptyArray());
    }
    int entry = FindEntryFromArray(data.GetTaggedValue(), key.GetTaggedValue());
    if (entry != -1) {
        return SetValueFromArray(thread, data, entry, value);
    }

    JSHandle<JSTaggedValue> localExportEntriesTv(thread, module->GetLocalExportEntries());
    if (!localExportEntriesTv->IsUndefined()) {
        StoreArrayByLocalExport(thread, localExportEntriesTv, value, key, data, module);
    }
}

int SourceTextModule::FindEntryFromArray(const JSTaggedValue &dictionary, const JSTaggedValue &key)
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *array = TaggedArray::Cast(dictionary.GetTaggedObject());
    size_t len = array->GetLength();
    for (size_t idx = 0; idx < len; idx = idx + 2) { // 2 means skip key
        if (JSTaggedValue::SameValue(key, array->Get(idx))) {
            return idx;
        }
    }
    return -1;
}

JSTaggedValue SourceTextModule::GetValueFromArray(const JSTaggedValue &dictionary, int entry)
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *array = TaggedArray::Cast(dictionary.GetTaggedObject());
    return array->Get(entry + 1);
}

void SourceTextModule::SetValueFromArray(JSThread *thread, const JSHandle<JSTaggedValue> &dictionary,
                                         int entry, const JSHandle<JSTaggedValue> &value)
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *array = TaggedArray::Cast(dictionary->GetTaggedObject());
    array->Set(thread, entry + 1, value.GetTaggedValue());
}

void SourceTextModule::SetExportName(JSThread *thread, const JSHandle<JSTaggedValue> &moduleRequest,
                                     const JSHandle<SourceTextModule> &module,
                                     CVector<std::string> &exportedNames, JSHandle<TaggedArray> &newExportStarSet)

{
    JSMutableHandle<SourceTextModule> requestedModule(thread, thread->GlobalConstants()->GetUndefined());
    JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
    if (moduleRecordName.IsUndefined()) {
        requestedModule.Update(SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest));
    } else {
        ASSERT(moduleRecordName.IsString());
        CVector<CString> npmStack;
        bool isNpm = false;
        requestedModule.Update(
            SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest, npmStack, isNpm));
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
                                                            JSHandle<JSTaggedValue>>> &resolveSet)
{
    auto globalConstants = thread->GlobalConstants();
    // a. Let importedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
    JSMutableHandle<SourceTextModule> importedModule(thread, thread->GlobalConstants()->GetUndefined());
    JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
    if (moduleRecordName.IsUndefined()) {
        importedModule.Update(SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest));
    } else {
        ASSERT(moduleRecordName.IsString());
        CVector<CString> npmStack;
        bool isNpm = false;
        importedModule.Update(
            SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest, npmStack, isNpm));
    }
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // b. Let resolution be ? importedModule.ResolveExport(exportName, resolveSet).
    JSHandle<JSTaggedValue> resolution =
        SourceTextModule::ResolveExport(thread, importedModule, exportName, resolveSet);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // c. If resolution is "ambiguous", return "ambiguous".
    if (resolution->IsString()) { // if resolution is string, resolution must be "ambiguous"
        return globalConstants->GetHandledAmbiguousString();
    }
    // d. If resolution is not null, then
    if (resolution->IsNull()) {
        return globalConstants->GetHandledNull();
    }
    // i. Assert: resolution is a ResolvedBinding Record.
    ASSERT(resolution->IsResolvedBinding());
    // ii. If starResolution is null, set starResolution to resolution.
    if (starResolution->IsNull()) {
        starResolution.Update(resolution.GetTaggedValue());
    } else {
        // 1. Assert: There is more than one * import that includes the requested name.
        // 2. If resolution.[[Module]] and starResolution.[[Module]] are not the same Module Record or
        // SameValue(
        //    resolution.[[BindingName]], starResolution.[[BindingName]]) is false, return "ambiguous".
        JSHandle<ResolvedBinding> resolutionBd = JSHandle<ResolvedBinding>::Cast(resolution);
        JSHandle<ResolvedBinding> starResolutionBd = JSHandle<ResolvedBinding>::Cast(starResolution);
        if ((!JSTaggedValue::SameValue(resolutionBd->GetModule(), starResolutionBd->GetModule())) ||
            (!JSTaggedValue::SameValue(
                resolutionBd->GetBindingName(), starResolutionBd->GetBindingName()))) {
            return globalConstants->GetHandledAmbiguousString();
        }
    }
    return resolution;
}

void SourceTextModule::UpdateNameDictionary(JSThread *thread, const JSHandle<JSTaggedValue> &exportName,
                                            JSHandle<JSTaggedValue> &data, JSHandle<SourceTextModule> &module,
                                            const JSHandle<JSTaggedValue> &value)
{
    int entry = FindEntryFromArray(data.GetTaggedValue(), exportName.GetTaggedValue());
    if (entry != -1) {
        SetValueFromArray(thread, data, entry, value);
        return;
    }
    JSHandle<TaggedArray> dataArray(data);
    size_t len = dataArray->GetLength();
    JSHandle<TaggedArray> newdataArray = TaggedArray::SetCapacity(thread, dataArray, len + 2); // 2 means key and value
    newdataArray->Set(thread, len, exportName);
    newdataArray->Set(thread, len + 1, value);
    module->SetNameDictionary(thread, newdataArray);
}


template <typename T>
void SourceTextModule::AddExportName(JSThread *thread, const JSTaggedValue &exportEntry,
                                     CVector<std::string> &exportedNames)
{
    if (!exportEntry.IsUndefined()) {
        JSMutableHandle<T> ee(thread, thread->GlobalConstants()->GetUndefined());
        if (!exportEntry.IsTaggedArray()) {
            // handle one ExportEntry
            ee.Update(exportEntry);
            std::string exportName =
                base::StringHelper::ToStdString(EcmaString::Cast(ee->GetExportName().GetTaggedObject()));
            exportedNames.emplace_back(exportName);
        } else {
            JSHandle<TaggedArray> exportEntries(thread, exportEntry);
            size_t exportEntriesLen = exportEntries->GetLength();
            for (size_t idx = 0; idx < exportEntriesLen; idx++) {
                ee.Update(exportEntries->Get(idx));
                // a. Assert: module provides the direct binding for this export.
                // b. Append e.[[ExportName]] to exportedNames.
                std::string exportName =
                    base::StringHelper::ToStdString(EcmaString::Cast(ee->GetExportName().GetTaggedObject()));
                exportedNames.emplace_back(exportName);
            }
        }
    }
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveLocalExport(JSThread *thread,
                                                             const JSHandle<JSTaggedValue> &exportEntry,
                                                             const JSHandle<JSTaggedValue> &exportName,
                                                             const JSHandle<SourceTextModule> &module)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> localName(thread, thread->GlobalConstants()->GetUndefined());
    if (exportEntry->IsLocalExportEntry()) {
        // handle one localExportEntry
        ee.Update(exportEntry);
        if (JSTaggedValue::SameValue(ee->GetExportName(), exportName.GetTaggedValue())) {
            localName.Update(ee->GetLocalName());
            return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedBindingRecord(module, localName));
        }
    } else {
        JSHandle<TaggedArray> localExportEntries(exportEntry);
        size_t localExportEntriesLen = localExportEntries->GetLength();
        for (size_t idx = 0; idx < localExportEntriesLen; idx++) {
            ee.Update(localExportEntries->Get(idx));
            // a. If SameValue(exportName, e.[[ExportName]]) is true, then
            if (JSTaggedValue::SameValue(ee->GetExportName(), exportName.GetTaggedValue())) {
                // i. Assert: module provides the direct binding for this export.
                // ii. Return ResolvedBinding Record { [[Module]]: module, [[BindingName]]: e.[[LocalName]] }.
                localName.Update(ee->GetLocalName());
                return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedBindingRecord(module, localName));
            }
        }
    }
    return thread->GlobalConstants()->GetHandledUndefined();
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveIndirectExport(JSThread *thread,
                                                                const JSHandle<JSTaggedValue> &exportEntry,
                                                                const JSHandle<JSTaggedValue> &exportName,
                                                                const JSHandle<SourceTextModule> &module,
                                                                CVector<std::pair<JSHandle<SourceTextModule>,
                                                                JSHandle<JSTaggedValue>>> &resolveSet)
{
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<IndirectExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    if (exportEntry->IsIndirectExportEntry()) {
        // handle one indirectExportEntry
        ee.Update(exportEntry);
        if (JSTaggedValue::SameValue(exportName.GetTaggedValue(), ee->GetExportName())) {
            moduleRequest.Update(ee->GetModuleRequest());
            JSMutableHandle<SourceTextModule> requestedModule(thread, thread->GlobalConstants()->GetUndefined());
            JSTaggedValue moduleRecordName = module->GetEcmaModuleRecordName();
            if (moduleRecordName.IsUndefined()) {
                requestedModule.Update(SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest));
            } else {
                ASSERT(moduleRecordName.IsString());
                CVector<CString> npmStack;
                bool isNpm = false;
                requestedModule.Update(
                    SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest,
                                                                         npmStack, isNpm));
            }
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            importName.Update(ee->GetImportName());
            return SourceTextModule::ResolveExport(thread, requestedModule, importName, resolveSet);
        }
    } else {
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
                    CVector<CString> npmStack;
                    bool isNpm = false;
                    requestedModule.Update(
                        SourceTextModule::HostResolveImportedModuleWithMerge(thread, module, moduleRequest,
                                                                             npmStack, isNpm));
                }
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
                // iii. Return importedModule.ResolveExport(e.[[ImportName]], resolveSet).
                importName.Update(ee->GetImportName());
                return SourceTextModule::ResolveExport(thread, requestedModule, importName, resolveSet);
            }
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
    if (indirectExportEntriesTv.IsIndirectExportEntry()) {
        ee.Update(indirectExportEntriesTv);
        // a. Let resolution be ? module.ResolveExport(e.[[ExportName]], « »).
        exportName.Update(ee->GetExportName());
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
        JSHandle<JSTaggedValue> resolution =
            SourceTextModule::ResolveExport(thread, module, exportName, resolveSet);
        // b. If resolution is null or "ambiguous", throw a SyntaxError exception.
        if (resolution->IsNull() || resolution->IsString()) {
            THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, "");
        }
        // c. Assert: resolution is a ResolvedBinding Record.
        ASSERT(resolution->IsResolvedBinding());
    } else {
        JSHandle<TaggedArray> indirectExportEntries(thread, indirectExportEntriesTv);
        size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
        for (size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
            ee.Update(indirectExportEntries->Get(idx));
            // a. Let resolution be ? module.ResolveExport(e.[[ExportName]], « »).
            exportName.Update(ee->GetExportName());
            CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
            JSHandle<JSTaggedValue> resolution =
                SourceTextModule::ResolveExport(thread, module, exportName, resolveSet);
            // b. If resolution is null or "ambiguous", throw a SyntaxError exception.
            if (resolution->IsNull() || resolution->IsString()) {
                THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, "");
            }
            // c. Assert: resolution is a ResolvedBinding Record.
            ASSERT(resolution->IsResolvedBinding());
        }
    }
}


JSTaggedValue SourceTextModule::FindByExport(const JSTaggedValue &exportEntriesTv, const JSTaggedValue &key,
                                             const JSTaggedValue &dictionary)
{
    DISALLOW_GARBAGE_COLLECTION;
    NameDictionary *dict = NameDictionary::Cast(dictionary.GetTaggedObject());
    if (exportEntriesTv.IsLocalExportEntry()) {
        LocalExportEntry *ee = LocalExportEntry::Cast(exportEntriesTv.GetTaggedObject());
        if (JSTaggedValue::SameValue(ee->GetLocalName(), key)) {
            JSTaggedValue exportName = ee->GetExportName();
            int entry = dict->FindEntry(exportName);
            if (entry != -1) {
                return dict->GetValue(entry);
            }
        }
    } else {
        TaggedArray *exportEntries = TaggedArray::Cast(exportEntriesTv.GetTaggedObject());
        size_t exportEntriesLen = exportEntries->GetLength();
        for (size_t idx = 0; idx < exportEntriesLen; idx++) {
            LocalExportEntry *ee = LocalExportEntry::Cast(exportEntries->Get(idx).GetTaggedObject());
            if (!JSTaggedValue::SameValue(ee->GetLocalName(), key)) {
                continue;
            }
            JSTaggedValue exportName = ee->GetExportName();
            int entry = dict->FindEntry(exportName);
            if (entry != -1) {
                return dict->GetValue(entry);
            }
        }
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue SourceTextModule::FindArrayByExport(const JSTaggedValue &exportEntriesTv, const JSTaggedValue &key,
                                                  const JSTaggedValue &dictionary)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (exportEntriesTv.IsLocalExportEntry()) {
        LocalExportEntry *ee = LocalExportEntry::Cast(exportEntriesTv.GetTaggedObject());
        if (JSTaggedValue::SameValue(ee->GetLocalName(), key)) {
            JSTaggedValue exportName = ee->GetExportName();
            int entry = FindEntryFromArray(dictionary, exportName);
            if (entry != -1) {
                return GetValueFromArray(dictionary, entry);
            }
        }
    } else {
        TaggedArray *exportEntries = TaggedArray::Cast(exportEntriesTv.GetTaggedObject());
        size_t exportEntriesLen = exportEntries->GetLength();
        for (size_t idx = 0; idx < exportEntriesLen; idx++) {
            LocalExportEntry *ee = LocalExportEntry::Cast(exportEntries->Get(idx).GetTaggedObject());
            if (!JSTaggedValue::SameValue(ee->GetLocalName(), key)) {
                continue;
            }
            JSTaggedValue exportName = ee->GetExportName();
            int entry = FindEntryFromArray(dictionary, exportName);
            if (entry != -1) {
                return GetValueFromArray(dictionary, entry);
            }
        }
    }
    return JSTaggedValue::Hole();
}

void SourceTextModule::StoreByLocalExport(JSThread *thread,
                                          const JSHandle<JSTaggedValue> &localExportEntriesTv,
                                          const JSHandle<JSTaggedValue> &value,
                                          const JSHandle<JSTaggedValue> &key,
                                          JSMutableHandle<NameDictionary> &dataDict)
{
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<LocalExportEntry> ee(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> exportName(thread, globalConstants->GetUndefined());
    if (localExportEntriesTv->IsLocalExportEntry()) {
        ee.Update(localExportEntriesTv);
        if (JSTaggedValue::SameValue(ee->GetLocalName(), key.GetTaggedValue())) {
            exportName.Update(ee->GetExportName());
            dataDict.Update(NameDictionary::Put(thread, dataDict, exportName, value,
                                                PropertyAttributes::Default()));
            return;
        }
    } else {
        JSHandle<TaggedArray> localExportEntries(localExportEntriesTv);
        size_t localExportEntriesLen = localExportEntries->GetLength();
        for (size_t idx = 0; idx < localExportEntriesLen; idx++) {
            ee.Update(localExportEntries->Get(idx));
            if (JSTaggedValue::SameValue(ee->GetLocalName(), key.GetTaggedValue())) {
                exportName.Update(ee->GetExportName());
                dataDict.Update(NameDictionary::Put(thread, dataDict, exportName, value,
                                                    PropertyAttributes::Default()));
                return;
            }
        }
    }
}

void SourceTextModule::StoreArrayByLocalExport(JSThread *thread,
                                               const JSHandle<JSTaggedValue> &localExportEntriesTv,
                                               const JSHandle<JSTaggedValue> &value,
                                               const JSHandle<JSTaggedValue> &key,
                                               JSMutableHandle<JSTaggedValue> &dataDict,
                                               JSHandle<SourceTextModule> &module)
{
    auto globalConstants = thread->GlobalConstants();
    JSMutableHandle<LocalExportEntry> ee(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> exportName(thread, globalConstants->GetUndefined());
    if (localExportEntriesTv->IsLocalExportEntry()) {
        ee.Update(localExportEntriesTv);
        if (JSTaggedValue::SameValue(ee->GetLocalName(), key.GetTaggedValue())) {
            exportName.Update(ee->GetExportName());
            UpdateNameDictionary(thread, exportName, dataDict, module, value);
            return;
        }
    } else {
        JSHandle<TaggedArray> localExportEntries(localExportEntriesTv);
        size_t localExportEntriesLen = localExportEntries->GetLength();
        for (size_t idx = 0; idx < localExportEntriesLen; idx++) {
            ee.Update(localExportEntries->Get(idx));
            if (JSTaggedValue::SameValue(ee->GetLocalName(), key.GetTaggedValue())) {
                exportName.Update(ee->GetExportName());
                UpdateNameDictionary(thread, exportName, dataDict, module, value);
                return;
            }
        }
    }
}
} // namespace panda::ecmascript
