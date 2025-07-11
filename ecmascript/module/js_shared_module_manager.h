/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MODULE_JS_SHARED_MODULE_MANAGER_H
#define ECMASCRIPT_MODULE_JS_SHARED_MODULE_MANAGER_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/module/module_manager_map.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
struct StateVisit {
    Mutex mutex;
    ConditionVariable cv;
    uint32_t threadId;
};
class SharedModuleManager {
public:
    static PUBLIC_API SharedModuleManager *GetInstance();

    void Initialize() {};

    void Destroy()
    {
        resolvedSharedModules_.Clear();
    }

    void Iterate(RootVisitor &v);

    StateVisit &FindModuleMutexWithLock(JSThread *thread, const JSHandle<SourceTextModule> &module);

    bool SearchInSModuleManager(JSThread *thread, const CString &recordName);

    JSHandle<SourceTextModule> GetSModule(JSThread *thread, const CString &recordName);

    JSHandle<SourceTextModule> PUBLIC_API TransferFromLocalToSharedModuleMapAndGetInsertedSModule(
        JSThread *thread, const JSHandle<SourceTextModule> &module);

    bool IsInstantiatedSModule(JSThread *thread, const JSHandle<SourceTextModule> &module);

    JSHandle<JSTaggedValue> GenerateFuncModule(JSThread *thread, const JSPandaFile *jsPandaFile,
        const CString &entryPoint, ClassKind classKind = ClassKind::NON_SENDABLE);

    JSHandle<ModuleNamespace> SModuleNamespaceCreate(JSThread *thread, const JSHandle<JSTaggedValue> &module,
                                                     const JSHandle<TaggedArray> &exports);

    void AddToResolvedModulesAndCreateSharedModuleMutex(JSThread *thread, const CString &recordName,
                                                        JSTaggedValue module);

    inline bool AddResolveImportedSModule(const CString &recordName, JSTaggedValue module)
    {
        return resolvedSharedModules_.Emplace(recordName, module);
    }

    inline void UpdateResolveImportedSModule(const CString &recordName, JSTaggedValue module)
    {
        resolvedSharedModules_.Insert(recordName, module);
    }
    void SharedNativeObjDestory();

    RecursiveMutex& GetSharedMutex()
    {
        return sharedMutex_;
    }

    inline uint32_t GetResolvedSharedModulesSize()
    {
        return resolvedSharedModules_.Size();
    }

    void AddSharedSerializeModule(JSThread *thread, JSHandle<TaggedArray> serializerArray, uint32_t idx)
    {
        resolvedSharedModules_.ForEach(
            [thread, &idx, &serializerArray](auto it) { serializerArray->Set(thread, idx++, it->second.Read()); });
    }

private:
    SharedModuleManager() = default;
    ~SharedModuleManager() = default;

    NO_COPY_SEMANTIC(SharedModuleManager);
    NO_MOVE_SEMANTIC(SharedModuleManager);

    bool SearchInSModuleManagerUnsafe(const CString &recordName);

    JSHandle<SourceTextModule> GetSModuleUnsafe(JSThread *thread, const CString &recordName);

    bool TryInsertInSModuleManager(JSThread *thread, const CString &recordName,
        const JSHandle<SourceTextModule> &moduleRecord);

    static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;
    ModuleManagerMap<CString> resolvedSharedModules_;
    CMap<CString, StateVisit> sharedModuleMutex_;
    Mutex mutex_;
    RecursiveMutex sharedMutex_;

    friend class SourceTextModule;
    friend class ModuleManager;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_JS_SHARED_MODULE_MANAGER_H
