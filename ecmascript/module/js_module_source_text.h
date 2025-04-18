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
#include "ecmascript/sendable_env.h"

namespace panda::ecmascript {
struct StateVisit;
enum class ModuleStatus : uint8_t {
    // don't change order
    UNINSTANTIATED = 0x0,
    PREINSTANTIATING,
    INSTANTIATING,
    INSTANTIATED,
    EVALUATING,
    EVALUATING_ASYNC,
    EVALUATED,
    ERRORED
};

enum class ModuleTypes : uint8_t {
    ECMA_MODULE = 0x01,
    CJS_MODULE,
    JSON_MODULE,
    NATIVE_MODULE,
    OHOS_MODULE,
    APP_MODULE,
    INTERNAL_MODULE,
    UNKNOWN
};

enum class LoadingTypes : uint8_t {
    STABLE_MODULE = 0x01,
    DYNAMITC_MODULE,
    OTHERS
};

enum class SharedTypes : uint8_t {
    UNSENDABLE_MODULE = 0x01,
    SENDABLE_FUNCTION_MODULE,
    SHARED_MODULE,
    TOTAL_KINDS,
};

class SourceTextModule final : public ModuleRecord {
public:
    static constexpr int UNDEFINED_INDEX = -1;
    static constexpr int MODULE_ERROR = 1;
    static constexpr size_t DEFAULT_DICTIONART_CAPACITY = 2;
    static constexpr size_t DEFAULT_ARRAY_CAPACITY = 2;
    static constexpr uint8_t DEREGISTER_MODULE_TAG = 1;
    static constexpr uint32_t FIRST_ASYNC_EVALUATING_ORDINAL = 2;
    static constexpr uint32_t NOT_ASYNC_EVALUATED = 0;
    static constexpr uint32_t ASYNC_EVALUATE_DID_FINISH = 1;
    static constexpr bool SHARED_MODULE_TAG = true;

    struct AsyncEvaluatingOrdinalCompare {
        bool operator()(const JSHandle<SourceTextModule> &lhs, const JSHandle<SourceTextModule> &rhs) const
        {
            return lhs->GetAsyncEvaluatingOrdinal() < rhs->GetAsyncEvaluatingOrdinal();
        }
    };
    using AsyncParentCompletionSet =
      CSet<JSHandle<SourceTextModule>, AsyncEvaluatingOrdinalCompare>;

    CAST_CHECK(SourceTextModule, IsSourceTextModule);

    // 15.2.1.16.2 GetExportedNames(exportStarSet)
    static CVector<std::string> GetExportedNames(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                                 const JSHandle<TaggedArray> &exportStarSet);

    // 15.2.1.16.3 ResolveExport(exportName, resolveVector)
    static JSHandle<JSTaggedValue> ResolveExport(JSThread *thread, const JSHandle<SourceTextModule> &module,
        const JSHandle<JSTaggedValue> &exportName,
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> &resolveVector);
    static JSHandle<JSTaggedValue> ResolveExportObject(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                                       const JSHandle<JSTaggedValue> &exportObject,
                                                       const JSHandle<JSTaggedValue> &exportName);
    static JSHandle<JSTaggedValue> ResolveNativeStarExport(JSThread *thread,
                                                           const JSHandle<SourceTextModule> &nativeModule,
                                                           const JSHandle<JSTaggedValue> &exportName);
    static JSHandle<JSTaggedValue> ResolveCjsStarExport(JSThread *thread,
                                                        const JSHandle<SourceTextModule> &cjsModule,
                                                        const JSHandle<JSTaggedValue> &exportName);

    // 15.2.1.16.4.2 ModuleDeclarationEnvironmentSetup ( module )
    static void ModuleDeclarationEnvironmentSetup(JSThread *thread, const JSHandle<SourceTextModule> &module);
    static void ModuleDeclarationArrayEnvironmentSetup(JSThread *thread, const JSHandle<SourceTextModule> &module);

    // 15.2.1.16.5.1 InnerModuleEvaluation ( module, stack, index )
    static int InnerModuleEvaluation(JSThread *thread, JSHandle<SourceTextModule> &moduleRecord,
        CVector<JSHandle<SourceTextModule>> &stack, CVector<JSHandle<SourceTextModule>> &errorStack,
        int index, const void *buffer = nullptr, size_t size = 0,
        const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static int InnerModuleEvaluationUnsafe(JSThread *thread,
        JSHandle<SourceTextModule> &module, CVector<JSHandle<SourceTextModule>> &stack,
        CVector<JSHandle<SourceTextModule>> &errorStack, int index, const void *buffer,
        size_t size, const ExecuteTypes &executeType);
    // 15.2.1.16.5.2 ModuleExecution ( module )
    static Expected<JSTaggedValue, bool> ModuleExecution(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                 const void *buffer = nullptr, size_t size = 0,
                                 const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    // 16.2.1.5.3.2 ExecuteAsyncModule ( module )
    static void ExecuteAsyncModule(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                   const void *buffer = nullptr, size_t size = 0,
                                   const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    // 16.2.1.5.3.3 GatherAvailableAncestors ( module, execList )
    static void GatherAvailableAncestors(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                         AsyncParentCompletionSet &execList);

    // 16.2.1.5.3.4 AsyncModuleExecutionFulfilled ( module )
    static void AsyncModuleExecutionFulfilled(JSThread *thread, const JSHandle<SourceTextModule> &module);

    // 16.2.1.5.3.5 AsyncModuleExecutionRejected ( module, error )
    static void AsyncModuleExecutionRejected(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                             JSTaggedValue error);

    static JSTaggedValue AsyncModuleFulfilledFunc(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue AsyncModuleRejectedFunc(EcmaRuntimeCallInfo *argv);
    static void AddAsyncParentModule(JSThread *thread, JSHandle<SourceTextModule> &module,
                                     JSHandle<SourceTextModule> &parent);
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
    static bool IsNativeModule(const CString &moduleRequestName);
    static ModuleTypes GetNativeModuleType(const CString &moduleRequestName);
    static JSTaggedValue GetRequireNativeModuleFunc(EcmaVM *vm, ModuleTypes moduleType);
    static JSHandle<JSTaggedValue> LoadNativeModuleImpl(EcmaVM *vm, JSThread *thread,
        const JSHandle<SourceTextModule> &requiredModule, ModuleTypes moduleType);
    static JSHandle<JSTaggedValue> LoadNativeModuleMayThrowError(JSThread *thread,
        const JSHandle<SourceTextModule> &requiredModule, ModuleTypes moduleType);
    static bool LoadNativeModule(JSThread *thread, const JSHandle<SourceTextModule> &requiredModule,
                                 ModuleTypes moduleType);
    bool CheckAndThrowModuleError(JSThread *thread);
    inline static bool IsNativeModule(ModuleTypes moduleType)
    {
        return moduleType == ModuleTypes::OHOS_MODULE ||
               moduleType == ModuleTypes::APP_MODULE ||
               moduleType == ModuleTypes::NATIVE_MODULE ||
               moduleType == ModuleTypes::INTERNAL_MODULE;
    }

    inline static CString GetResolveErrorReason(const JSHandle<JSTaggedValue> &resolution)
    {
        ASSERT(resolution->IsNull() || resolution->IsString());
        return resolution->IsNull() ? "' does not provide an export name '"
                                    : "' provide an ambiguous export name '";
    }

    inline static bool IsCjsModule(ModuleTypes moduleType)
    {
        return moduleType == ModuleTypes::CJS_MODULE;
    }

    inline static bool IsJsonModule(ModuleTypes moduleType)
    {
        return moduleType == ModuleTypes::JSON_MODULE;
    }

    inline static bool IsModuleInSharedHeap(JSHandle<SourceTextModule> currentModule)
    {
        return currentModule->GetSharedType() > SharedTypes::UNSENDABLE_MODULE;
    }

    inline static bool IsSharedModule(JSHandle<SourceTextModule> currentModule)
    {
        return currentModule->GetSharedType() == SharedTypes::SHARED_MODULE;
    }

    static bool IsEvaluatedModule(JSThread *thread, StateVisit &stateVisit,
        const JSHandle<SourceTextModule> &module);

    static ModuleStatus GetModuleEvaluatingType(JSThread *thread, StateVisit &stateVisit,
        const JSHandle<SourceTextModule> &module);

    inline static bool IsSendableFunctionModule(JSTaggedValue currentModule)
    {
        return SourceTextModule::Cast(currentModule.GetTaggedObject())->GetSharedType() ==
            SharedTypes::SENDABLE_FUNCTION_MODULE;
    }

    inline bool *GetLazyImportStatusArray()
    {
        return reinterpret_cast<bool *>(GetLazyImportStatus());
    }

    inline void SetLazyImportArray(bool *lazyImportArray)
    {
        if (lazyImportArray != nullptr) {
            DestoryLazyImportArray();
        }
        SetLazyImportStatus(ToUintPtr(lazyImportArray));
    }

    inline void DestoryLazyImportArray()
    {
        delete GetLazyImportStatusArray();
        SetLazyImportStatus(ToUintPtr(nullptr));
    }

    inline bool IsLazyImportModule(size_t index)
    {
        bool *lazyArray = GetLazyImportStatusArray();
        if (lazyArray == nullptr) {
            return false;
        }
        return lazyArray[index];
    }

    inline CString GetEcmaModuleFilenameString() const
    {
        CString *fileName = reinterpret_cast<CString *>(GetEcmaModuleFilename());
        if (fileName == nullptr) {
            return CString();
        }
        return *fileName;
    }

    inline CString GetEcmaModuleRecordNameString() const
    {
        CString *recordName = reinterpret_cast<CString *>(GetEcmaModuleRecordName());
        if (recordName == nullptr) {
            return CString();
        }
        return *recordName;
    }

    inline void SetEcmaModuleFilenameString(const CString &fileName)
    {
        CString *ptr = new CString(fileName);
        DestoryEcmaModuleFilenameString();
        SetEcmaModuleFilename(ToUintPtr(ptr));
    }

    inline void SetEcmaModuleRecordNameString(const CString &recordName)
    {
        CString *ptr = new CString(recordName);
        DestoryEcmaModuleRecordNameString();
        SetEcmaModuleRecordName(ToUintPtr(ptr));
    }

    inline void DestoryEcmaModuleFilenameString()
    {
        CString *ptr = reinterpret_cast<CString *>(GetEcmaModuleFilename());
        delete ptr;
        SetEcmaModuleFilename(ToUintPtr(nullptr));
    }

    inline void DestoryEcmaModuleRecordNameString()
    {
        CString *ptr = reinterpret_cast<CString *>(GetEcmaModuleRecordName());
        delete ptr;
        SetEcmaModuleRecordName(ToUintPtr(nullptr));
    }

    static constexpr size_t SOURCE_TEXT_MODULE_OFFSET = ModuleRecord::SIZE;
    ACCESSORS(Environment, SOURCE_TEXT_MODULE_OFFSET, NAMESPACE_OFFSET);
    ACCESSORS(Namespace, NAMESPACE_OFFSET, MODULE_REQUESTS_OFFSET);
    ACCESSORS(ModuleRequests, MODULE_REQUESTS_OFFSET, REQUESTED_MODULES_OFFSET);
    ACCESSORS(RequestedModules, REQUESTED_MODULES_OFFSET, IMPORT_ENTRIES_OFFSET);
    ACCESSORS(ImportEntries, IMPORT_ENTRIES_OFFSET, LOCAL_EXPORT_ENTTRIES_OFFSET);
    ACCESSORS(LocalExportEntries, LOCAL_EXPORT_ENTTRIES_OFFSET, INDIRECT_EXPORT_ENTTRIES_OFFSET);
    ACCESSORS(IndirectExportEntries, INDIRECT_EXPORT_ENTTRIES_OFFSET, START_EXPORT_ENTTRIES_OFFSET);
    ACCESSORS(StarExportEntries, START_EXPORT_ENTTRIES_OFFSET, NAME_DICTIONARY_OFFSET);
    ACCESSORS(NameDictionary, NAME_DICTIONARY_OFFSET, CYCLE_ROOT_OFFSET);
    ACCESSORS(CycleRoot, CYCLE_ROOT_OFFSET, TOP_LEVEL_CAPABILITY_OFFSET);
    ACCESSORS(TopLevelCapability, TOP_LEVEL_CAPABILITY_OFFSET, ASYNC_PARENT_MODULES_OFFSET);
    ACCESSORS(AsyncParentModules, ASYNC_PARENT_MODULES_OFFSET, SENDABLE_ENV_OFFSET);
    ACCESSORS(SendableEnv, SENDABLE_ENV_OFFSET, EXCEPTION_OFFSET);
    ACCESSORS(Exception, EXCEPTION_OFFSET, DFS_ANCESTOR_INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(DFSAncestorIndex, int32_t, DFS_ANCESTOR_INDEX_OFFSET, DFS_INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(DFSIndex, int32_t, DFS_INDEX_OFFSET, ASYNC_EVALUATION_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(AsyncEvaluatingOrdinal, uint32_t, ASYNC_EVALUATION_OFFSET, PENDING_DEPENDENCIES_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(PendingAsyncDependencies,
        int32_t, PENDING_DEPENDENCIES_OFFSET, LAYZ_IMPORT_STATUS_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(LazyImportStatus, uintptr_t, LAYZ_IMPORT_STATUS_OFFSET, ECMA_MODULE_FILENAME);
    ACCESSORS_PRIMITIVE_FIELD(EcmaModuleFilename, uintptr_t, ECMA_MODULE_FILENAME, ECMA_MODULE_RECORDNAME);
    ACCESSORS_PRIMITIVE_FIELD(EcmaModuleRecordName, uintptr_t, ECMA_MODULE_RECORDNAME, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)

    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t STATUS_BITS = 3;
    static constexpr size_t MODULE_TYPE_BITS = 4;
    static constexpr size_t IS_NEW_BC_VERSION_BITS = 1;
    static constexpr size_t HASTLA_BITS = 1;
    static constexpr size_t LOADING_TYPE_BITS = 3;
    static constexpr uint16_t REGISTER_COUNTS = 16;
    static constexpr size_t IS_SHARED_TYPE_BITS = 2;

    FIRST_BIT_FIELD(BitField, Status, ModuleStatus, STATUS_BITS)
    NEXT_BIT_FIELD(BitField, Types, ModuleTypes, MODULE_TYPE_BITS, Status)
    NEXT_BIT_FIELD(BitField, IsNewBcVersion, bool, IS_NEW_BC_VERSION_BITS, Types)
    NEXT_BIT_FIELD(BitField, HasTLA, bool, HASTLA_BITS, IsNewBcVersion)
    NEXT_BIT_FIELD(BitField, LoadingTypes, LoadingTypes, LOADING_TYPE_BITS, HasTLA)
    NEXT_BIT_FIELD(BitField, RegisterCounts, uint16_t, REGISTER_COUNTS, LoadingTypes)
    NEXT_BIT_FIELD(BitField, SharedType, SharedTypes, IS_SHARED_TYPE_BITS, RegisterCounts)

    static_assert(static_cast<size_t>(SharedTypes::TOTAL_KINDS) <= (1 << IS_SHARED_TYPE_BITS));

    DECL_DUMP()
    DECL_VISIT_OBJECT(SOURCE_TEXT_MODULE_OFFSET, DFS_ANCESTOR_INDEX_OFFSET)

    // 15.2.1.16.5 Evaluate()
    static JSTaggedValue Evaluate(JSThread *thread, const JSHandle<SourceTextModule> &module,
                         const void *buffer = nullptr, size_t size = 0,
                         const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    // 15.2.1.16.4 Instantiate()
    static int PUBLIC_API Instantiate(JSThread *thread,
                                      const JSHandle<JSTaggedValue> &moduleHdl,
                                      const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static bool EvaluateNativeModule(JSThread *thread, JSHandle<SourceTextModule> nativeModule,
                                     ModuleTypes moduleType);

    JSTaggedValue GetModuleValue(JSThread *thread, int32_t index, bool isThrow);
    static void StoreModuleValue(JSThread *thread, const JSHandle<SourceTextModule> &module, int32_t index,
                                 const JSHandle<JSTaggedValue> &value);

    JSTaggedValue GetModuleValue(JSThread *thread, JSTaggedValue key, bool isThrow);
    static void StoreModuleValue(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                 const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static JSTaggedValue GetValueFromExportObject(JSThread *thread, JSHandle<JSTaggedValue> &exportObject,
        int32_t index);

    static JSHandle<JSTaggedValue> ResolveIndirectExport(JSThread *thread, const JSHandle<JSTaggedValue> &exportEntry,
                                                         const JSHandle<JSTaggedValue> &exportName,
                                                         const JSHandle<SourceTextModule> &module,
                                                         CVector<std::pair<JSHandle<SourceTextModule>,
                                                         JSHandle<JSTaggedValue>>> &resolveVector);
    static CString GetModuleName(JSTaggedValue currentModule);

    static bool IsDynamicModule(LoadingTypes types);

    // taskpool
    static std::optional<std::set<uint32_t>> GetConcurrentRequestedModules(const JSHandle<Method> &method);
    static int EvaluateForConcurrent(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                     const JSHandle<Method> &method);
    static int ModuleEvaluation(JSThread *thread, const JSHandle<SourceTextModule> &module,
                                int index, const JSHandle<Method> &method);
    static void CheckCircularImportTool(JSThread *thread, const CString &circularModuleRecordName,
                                        CList<CString> &referenceList, bool printOtherCircular = false);

    static void CheckResolvedBinding(JSThread *thread, const JSHandle<SourceTextModule> &module);
    static bool IsCircular(const CList<CString> &referenceList, const CString &requiredModuleName);
    static void PrintCircular(const CList<CString> &referenceList, Level level);
    static void SearchCircularImport(JSThread *thread, const CString &circularModuleRecordName,
                                     const JSHandle<SourceTextModule> &module, CList<CString> &referenceList,
                                     CString &requiredModuleName, bool printOtherCircular);
    static void CheckResolvedIndexBinding(JSThread *thread, const JSHandle<SourceTextModule> &module);
    static void SetExportName(JSThread *thread, const JSHandle<SourceTextModule> requestedModule,
                              CVector<std::string> &exportedNames, JSHandle<TaggedArray> &newExportStarSet);
    static void RecordEvaluatedOrError(JSThread *thread, JSHandle<SourceTextModule> module);
    static JSHandle<SourceTextModule> GetRequestedModule(JSThread *thread,
                                                         const JSHandle<TaggedArray> requestedModules,
                                                         uint32_t idx);
    static bool PreModuleInstantiation(JSThread *thread,
                                       JSHandle<SourceTextModule> module,
                                       const ExecuteTypes &executeType);
    static int FinishModuleInstantiation(JSThread *thread,
                                         JSHandle<SourceTextModule> module,
                                         CVector<JSHandle<SourceTextModule>> &stack,
                                         int index);

private:
    static JSHandle<JSTaggedValue> GetStarResolution(JSThread *thread, const JSHandle<JSTaggedValue> &exportName,
                                                     const JSHandle<SourceTextModule> importedModule,
                                                     JSMutableHandle<JSTaggedValue> &starResolution,
                                                     CVector<std::pair<JSHandle<SourceTextModule>,
                                                     JSHandle<JSTaggedValue>>> &resolveVector);
    template <typename T>
    static void AddExportName(JSThread *thread, const JSTaggedValue &exportEntry, CVector<std::string> &exportedNames);
    static JSHandle<JSTaggedValue> ResolveLocalExport(JSThread *thread, const JSHandle<JSTaggedValue> &exportEntry,
                                                      const JSHandle<JSTaggedValue> &exportName,
                                                      const JSHandle<SourceTextModule> &module);
    static JSHandle<JSTaggedValue> ResolveElementOfObject(JSThread *thread,
                                                         const JSHandle<JSHClass> &hclass,
                                                         const JSHandle<JSTaggedValue> &exportName,
                                                         const JSHandle<SourceTextModule> &module);
    static bool CheckCircularImport(const JSHandle<SourceTextModule> &module,
                                    const JSHandle<JSTaggedValue> &exportName,
                                    CVector<std::pair<JSHandle<SourceTextModule>,
                                    JSHandle<JSTaggedValue>>> &resolveVector);
    static JSTaggedValue FindByExport(const JSTaggedValue &exportEntriesTv, const JSTaggedValue &key,
                                      const JSTaggedValue &dictionary);
    static void DFSModuleInstantiation(JSHandle<SourceTextModule> &module,
                                       CVector<JSHandle<SourceTextModule>> &stack);
    static int HandleInstantiateException(JSHandle<SourceTextModule> &module,
                                          const CVector<JSHandle<SourceTextModule>> &stack, int result);
    static void HandlePreInstantiationException(JSThread *thread, JSHandle<SourceTextModule> module);
    static void HandleEvaluateResult(JSThread *thread, JSHandle<SourceTextModule> &module,
                                     JSHandle<JSPromise> &capability,
                                     const CVector<JSHandle<SourceTextModule>> &stack,
                                     const CVector<JSHandle<SourceTextModule>> &errorStack);
    static void HandleConcurrentEvaluateResult(JSThread *thread, JSHandle<SourceTextModule> &module,
                                               const CVector<JSHandle<SourceTextModule>> &stack,
                                               const CVector<JSHandle<SourceTextModule>> &errorStack);
    bool IsAsyncEvaluating();
    static void HandleEvaluateException(JSThread *thread,
                                        const CVector<JSHandle<SourceTextModule>> &stack,
                                        JSHandle<JSTaggedValue> exception);
    static void HandleErrorStack(JSThread *thread, const CVector<JSHandle<SourceTextModule>> &errorStack);
    static void SetExceptionToModule(JSThread *thread, JSHandle<SourceTextModule> module,
                                     JSTaggedValue exception);
    friend class EcmaModuleTest;
    friend class SharedModuleManager;
};

class ResolvedBinding final : public Record {
public:
    CAST_CHECK(ResolvedBinding, IsResolvedBinding);

    static constexpr size_t MODULE_OFFSET = Record::SIZE;
    ACCESSORS(Module, MODULE_OFFSET, BINDING_NAME_OFFSET);
    ACCESSORS(BindingName, BINDING_NAME_OFFSET, SIZE);

    DECL_DUMP()
    DECL_VISIT_OBJECT(MODULE_OFFSET, SIZE)
};
class ResolvedIndexBinding final : public Record {
public:
    CAST_CHECK(ResolvedIndexBinding, IsResolvedIndexBinding);

    static constexpr size_t MODULE_OFFSET = Record::SIZE;
    ACCESSORS(Module, MODULE_OFFSET, INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Index, int32_t, INDEX_OFFSET, END_OFFSET);
    DEFINE_ALIGN_SIZE(END_OFFSET);

    DECL_DUMP()
    DECL_VISIT_OBJECT(MODULE_OFFSET, INDEX_OFFSET)
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_MODULE_SOURCE_TEXT_H
