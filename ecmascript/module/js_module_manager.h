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

#ifndef ECMASCRIPT_MODULE_JS_MODULE_MANAGER_H
#define ECMASCRIPT_MODULE_JS_MODULE_MANAGER_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/module/module_manager_map.h"

namespace panda::ecmascript {
enum class ModuleExecuteMode {
    ExecuteZipMode,
    ExecuteBufferMode
};
class ModuleManager {
public:
    explicit ModuleManager(EcmaVM *vm);
    ~ModuleManager()
    {
        resolvedModules_.Clear();
    }

    JSHandle<SourceTextModule> GetImportedModule(const CString &referencing);
    JSHandle<SourceTextModule> PUBLIC_API HostGetImportedModule(const CString &referencing);
    JSTaggedValue HostGetImportedModule(void *src);
    bool IsLocalModuleLoaded(const CString& referencing);
    bool IsSharedModuleLoaded(const CString &referencing);
    bool IsModuleLoaded(const CString &referencing);

    bool IsEvaluatedModule(const CString &referencing);
    bool IsInstantiatedModule(const CString &referencing);
    bool IsLocalModuleInstantiated(const CString &referencing);
    bool NeedExecuteModule(const CString &referencing);

    JSHandle<JSTaggedValue> LoadNativeModule(JSThread *thread, const CString &key);

    JSHandle<JSTaggedValue> ExecuteNativeModuleMayThrowError(JSThread *thread, const CString &recordName);

    JSHandle<JSTaggedValue> ExecuteNativeModule(JSThread *thread, const CString &recordName);

    JSHandle<JSTaggedValue> ExecuteJsonModule(JSThread *thread, const CString &recordName,
                                              const CString &filename, const JSPandaFile *jsPandaFile);
    JSHandle<JSTaggedValue> ExecuteCjsModule(JSThread *thread, const CString &recordName,
                                             const JSPandaFile *jsPandaFile);

    JSHandle<JSTaggedValue> GenerateSendableFuncModule(const JSHandle<JSTaggedValue> &module);

    JSHandle<JSTaggedValue> TryGetImportedModule(const CString& referencing);
    void Iterate(RootVisitor &v);

    ModuleExecuteMode GetExecuteMode() const
    {
        return isExecuteBuffer_.load(std::memory_order_acquire);
    }
    inline void SetExecuteMode(ModuleExecuteMode mode)
    {
        isExecuteBuffer_.store(mode, std::memory_order_release);
    }

    static CString PUBLIC_API GetRecordName(const JSThread *thread, JSTaggedValue module);
    static int GetExportObjectIndex(EcmaVM *vm, JSHandle<SourceTextModule> ecmaModule, const CString &key);

    uint32_t NextModuleAsyncEvaluatingOrdinal()
    {
        uint32_t ordinal = nextModuleAsyncEvaluatingOrdinal_++;
        return ordinal;
    }

    inline void AddResolveImportedModule(const CString &recordName, JSTaggedValue module)
    {
        resolvedModules_.Emplace(recordName, module);
    }

    inline void UpdateResolveImportedModule(const CString &recordName, JSTaggedValue module)
    {
        resolvedModules_.Insert(recordName, module);
    }

    void NativeObjDestory()
    {
        resolvedModules_.ForEach([](auto it) {
            CString key = it->first;
            ASSERT(!key.empty());
            GCRoot &root = it->second;
            JSTaggedValue module = root.Read();
            SourceTextModule::Cast(module)->DestoryLazyImportArray();
            SourceTextModule::Cast(module)->DestoryEcmaModuleFilenameString();
            SourceTextModule::Cast(module)->DestoryEcmaModuleRecordNameString();
        });
    }

    inline uint32_t GetResolvedModulesSize() const
    {
        return resolvedModules_.Size();
    }

    inline void AddNormalSerializeModule(JSThread *thread, JSHandle<TaggedArray> serializerArray, uint32_t idx)
    {
        resolvedModules_.ForEach(
            [thread, &idx, serializerArray](auto it) { serializerArray->Set(thread, idx++, it->second.Read()); });
    }

    inline bool IsVMBundlePack()
    {
        return vm_->IsBundlePack();
    }
    static JSTaggedValue CreateModuleManagerNativePointer(JSThread *thread);
    void SyncModuleExecuteMode(JSThread *thread);

    // fast path ldexternalmodulevar for jit
    static bool CheckModuleValueOutterResolved(JSThread *thread, int32_t index, JSFunction *jsFunc);
    static JSTaggedValue GetExternalModuleVarFastPathForJIT(JSThread *thread, int32_t index, JSFunction *jsFunc);

    // for ut
    void ClearResolvedModules()
    {
        resolvedModules_.Clear();
    }

private:
    NO_COPY_SEMANTIC(ModuleManager);
    NO_MOVE_SEMANTIC(ModuleManager);

    void RemoveModuleFromCache(const CString &recordName);

    void RemoveModuleNameFromList(const CString &recordName);

    static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;

    uint32_t nextModuleAsyncEvaluatingOrdinal_{SourceTextModule::FIRST_ASYNC_EVALUATING_ORDINAL};

    EcmaVM *vm_ {nullptr};
    ModuleManagerMap<CString> resolvedModules_;
    std::atomic<ModuleExecuteMode> isExecuteBuffer_ {ModuleExecuteMode::ExecuteZipMode};

    friend class EcmaVM;
    friend class PatchLoader;
    friend class ModuleDeregister;
    friend class SharedModuleManager;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_JS_MODULE_MANAGER_H
