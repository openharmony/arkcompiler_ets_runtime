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

#ifndef ECMASCRIPT_MODULE_JS_MODULE_SOURCE_TEXT_H
#define ECMASCRIPT_MODULE_JS_MODULE_SOURCE_TEXT_H

#include "ecmascript/base/string_helper.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/module/js_module_record.h"
#include "ecmascript/module/js_module_entry.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
enum class ModuleStatus : uint8_t { UNINSTANTIATED = 0x01, INSTANTIATING, INSTANTIATED, EVALUATING, EVALUATED };
enum class ModuleTypes : uint8_t { ECMAMODULE = 0x01, CJSMODULE, UNKNOWN};
enum class ModuleModes : uint8_t { ARRAYMODE = 0x01, DICTIONARYMODE};

class SourceTextModule final : public ModuleRecord {
public:
    static constexpr int UNDEFINED_INDEX = -1;

    CAST_CHECK(SourceTextModule, IsSourceTextModule);

    // 15.2.1.17 Runtime Semantics: HostResolveImportedModule ( referencingModule, specifier )
    static JSHandle<SourceTextModule> HostResolveImportedModule(JSThread *thread,
                                                                const JSHandle<SourceTextModule> &module,
                                                                const JSHandle<JSTaggedValue> &moduleRequest);
    static JSHandle<SourceTextModule> HostResolveImportedModuleWithMerge(
        JSThread *thread, const JSHandle<SourceTextModule> &module, const JSHandle<JSTaggedValue> &moduleRequest);

    // 15.2.1.16.2 GetExportedNames(exportStarSet)
    static CVector<std::string> GetExportedNames(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                                 const JSHandle<TaggedArray> &exportStarSet);

    // 15.2.1.16.3 ResolveExport(exportName, resolveSet)
    static JSHandle<JSTaggedValue> ResolveExport(JSThread *thread, const JSHandle<SourceTextModule> &module,
        const JSHandle<JSTaggedValue> &exportName,
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> &resolveSet);

    // 15.2.1.16.4.1 InnerModuleInstantiation ( module, stack, index )
    static int InnerModuleInstantiation(JSThread *thread, const JSHandle<ModuleRecord> &moduleRecord,
                                        CVector<JSHandle<SourceTextModule>> &stack, int index);

    // 15.2.1.16.4.2 ModuleDeclarationEnvironmentSetup ( module )
    static void ModuleDeclarationEnvironmentSetup(JSThread *thread, const JSHandle<SourceTextModule> &module);

    // 15.2.1.16.5.1 InnerModuleEvaluation ( module, stack, index )
    static int InnerModuleEvaluation(JSThread *thread, const JSHandle<ModuleRecord> &moduleRecord,
        CVector<JSHandle<SourceTextModule>> &stack, int index, const void *buffer = nullptr, size_t size = 0);

    // 15.2.1.16.5.2 ModuleExecution ( module )
    static void ModuleExecution(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                const void *buffer = nullptr, size_t size = 0);

    // 15.2.1.18 Runtime Semantics: GetModuleNamespace ( module )
    static JSHandle<JSTaggedValue> GetModuleNamespace(JSThread *thread, const JSHandle<SourceTextModule> &module);

    static void AddImportEntry(JSThread *thread, const JSHandle<SourceTextModule> &module,
                               const JSHandle<ImportEntry> &importEntry, size_t idx, uint32_t len);
    static void AddLocalExportEntry(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                    const JSHandle<LocalExportEntry> &exportEntry, size_t idx, uint32_t len);
    static void AddIndirectExportEntry(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                       const JSHandle<IndirectExportEntry> &exportEntry, size_t idx, uint32_t len);
    static void AddStarExportEntry(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                   const JSHandle<StarExportEntry> &exportEntry, size_t idx, uint32_t len);
    static constexpr size_t SOURCE_TEXT_MODULE_OFFSET = ModuleRecord::SIZE;
    ACCESSORS(Environment, SOURCE_TEXT_MODULE_OFFSET, NAMESPACE_OFFSET);
    ACCESSORS(Namespace, NAMESPACE_OFFSET, ECMA_MODULE_FILENAME);
    ACCESSORS(EcmaModuleFilename, ECMA_MODULE_FILENAME, ECMA_MODULE_RECORDNAME);
    ACCESSORS(EcmaModuleRecordName, ECMA_MODULE_RECORDNAME, REQUESTED_MODULES_OFFSET);
    ACCESSORS(NpmKey, REQUESTED_MODULES_OFFSET, NPM_KEY_OFFSET);
    ACCESSORS(RequestedModules, NPM_KEY_OFFSET, IMPORT_ENTRIES_OFFSET);
    ACCESSORS(ImportEntries, IMPORT_ENTRIES_OFFSET, LOCAL_EXPORT_ENTTRIES_OFFSET);
    ACCESSORS(LocalExportEntries, LOCAL_EXPORT_ENTTRIES_OFFSET, INDIRECT_EXPORT_ENTTRIES_OFFSET);
    ACCESSORS(IndirectExportEntries, INDIRECT_EXPORT_ENTTRIES_OFFSET, START_EXPORT_ENTTRIES_OFFSET);
    ACCESSORS(StarExportEntries, START_EXPORT_ENTTRIES_OFFSET, NAME_DICTIONARY_OFFSET);
    ACCESSORS(NameDictionary, NAME_DICTIONARY_OFFSET, EVALUATION_ERROR_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(EvaluationError, int32_t, EVALUATION_ERROR_OFFSET, DFS_ANCESTOR_INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(DFSAncestorIndex, int32_t, DFS_ANCESTOR_INDEX_OFFSET, DFS_INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(DFSIndex, int32_t, DFS_INDEX_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)

    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t STATUS_BITS = 3;
    static constexpr size_t MODULE_TYPE = 2;
    static constexpr size_t ELEMENT_MODE = 2;
    FIRST_BIT_FIELD(BitField, Status, ModuleStatus, STATUS_BITS)
    NEXT_BIT_FIELD(BitField, Types, ModuleTypes, MODULE_TYPE, Status)
    NEXT_BIT_FIELD(BitField, Modes, ModuleModes, ELEMENT_MODE, Types)

    DECL_DUMP()
    DECL_VISIT_OBJECT(SOURCE_TEXT_MODULE_OFFSET, EVALUATION_ERROR_OFFSET)

    // 15.2.1.16.5 Evaluate()
    static int Evaluate(JSThread *thread, const JSHandle<SourceTextModule> &module,
                        const void *buffer = nullptr, size_t size = 0);

    // 15.2.1.16.4 Instantiate()
    static int Instantiate(JSThread *thread, const JSHandle<SourceTextModule> &module);

    JSTaggedValue GetModuleValue(JSThread *thread, JSTaggedValue key, bool isThrow);
    static JSTaggedValue GetModuleValueFromArray(const JSTaggedValue &sourceTextmodule, JSTaggedValue &key);
    void StoreModuleValue(JSThread *thread, const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static constexpr size_t DEFAULT_DICTIONART_CAPACITY = 2;
    static constexpr size_t DEFAULT_ARRAY_CAPACITY = 2;
private:
    void StoreModuleValueFromArray(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &value);
    static int FindEntryFromArray(const JSTaggedValue &dictionary, const JSTaggedValue &key);
    static JSTaggedValue GetValueFromArray(const JSTaggedValue &dictionary, int entry);
    static void SetValueFromArray(JSThread *thread, const JSHandle<JSTaggedValue> &dictionary,
                                  int entry, const JSHandle<JSTaggedValue> &value);
    static void SetExportName(JSThread *thread,
                              const JSHandle<JSTaggedValue> &moduleRequest, const JSHandle<SourceTextModule> &module,
                              CVector<std::string> &exportedNames, JSHandle<TaggedArray> &newExportStarSet);
    static JSHandle<JSTaggedValue> GetStarResolution(JSThread *thread, const JSHandle<JSTaggedValue> &exportName,
                                                     const JSHandle<JSTaggedValue> &moduleRequest,
                                                     const JSHandle<SourceTextModule> &module,
                                                     JSMutableHandle<JSTaggedValue> &starResolution,
                                                     CVector<std::pair<JSHandle<SourceTextModule>,
                                                     JSHandle<JSTaggedValue>>> &resolveSet);
    static void UpdateNameDictionary(JSThread *thread, const JSHandle<JSTaggedValue> &exportName,
                                     JSHandle<JSTaggedValue> &data, JSHandle<SourceTextModule> &module,
                                     const JSHandle<JSTaggedValue> &value);
    template <typename T>
    static void AddExportName(JSThread *thread, const JSTaggedValue &exportEntry, CVector<std::string> &exportedNames);
    static JSHandle<JSTaggedValue> ResolveLocalExport(JSThread *thread, const JSHandle<JSTaggedValue> &exportEntry,
                                                      const JSHandle<JSTaggedValue> &exportName,
                                                      const JSHandle<SourceTextModule> &module);
    static JSHandle<JSTaggedValue> ResolveIndirectExport(JSThread *thread, const JSHandle<JSTaggedValue> &exportEntry,
                                                         const JSHandle<JSTaggedValue> &exportName,
                                                         const JSHandle<SourceTextModule> &module,
                                                         CVector<std::pair<JSHandle<SourceTextModule>,
                                                         JSHandle<JSTaggedValue>>> &resolveSet);
    static void CheckResolvedBinding(JSThread *thread, const JSHandle<SourceTextModule> &module);
    static JSTaggedValue FindByExport(const JSTaggedValue &exportEntriesTv, const JSTaggedValue &key,
                                      const JSTaggedValue &dictionary);
    static JSTaggedValue FindArrayByExport(const JSTaggedValue &exportEntriesTv, const JSTaggedValue &key,
                                           const JSTaggedValue &dictionary);
    static void StoreByLocalExport(JSThread *thread, const JSHandle<JSTaggedValue> &localExportEntriesTv,
                                   const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &key,
                                   JSMutableHandle<NameDictionary> &dataDict);
    static void StoreArrayByLocalExport(JSThread *thread, const JSHandle<JSTaggedValue> &localExportEntriesTv,
                                        const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &key,
                                        JSMutableHandle<JSTaggedValue> &dataDict, JSHandle<SourceTextModule> &module);
};

class ResolvedBinding final : public Record {
public:
    CAST_CHECK(ResolvedBinding, IsResolvedBinding);

    static constexpr size_t RESOLVED_BINDING_OFFSET = Record::SIZE;
    ACCESSORS(Module, RESOLVED_BINDING_OFFSET, MODULE_OFFSET);
    ACCESSORS(BindingName, MODULE_OFFSET, SIZE);

    DECL_DUMP()
    DECL_VISIT_OBJECT(RESOLVED_BINDING_OFFSET, SIZE)
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_MODULE_SOURCE_TEXT_H
