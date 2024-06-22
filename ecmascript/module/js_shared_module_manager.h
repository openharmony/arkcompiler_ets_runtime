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
#include "ecmascript/napi/jsnapi_helper.h"
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
    static SharedModuleManager *GetInstance();

    void Initialize(const EcmaVM *vm)
    {
        resolvedSharedModules_ = NameDictionary::CreateInSharedHeap(vm->GetJSThread(),
            DEAULT_DICTIONART_CAPACITY).GetTaggedValue();
    }

    void Destroy()
    {
        resolvedSharedModules_ = JSTaggedValue::Hole();
    }

    JSTaggedValue GetSendableModuleValue(JSThread *thread, int32_t index, JSTaggedValue jsFunc);

    JSTaggedValue GetSendableModuleValueImpl(JSThread *thread, int32_t index, JSTaggedValue currentModule) const;

    void Iterate(const RootVisitor &v);

    JSHandle<JSTaggedValue> ResolveImportedModule(JSThread *thread, const CString &referencingModule,
                                                  bool executeFromJob);

    JSHandle<JSTaggedValue> ResolveImportedModuleWithMerge(JSThread *thread, const CString &fileName,
                                                           const CString &recordName, bool executeFromJob);

    StateVisit &findModuleMutexWithLock(JSThread *thread, const JSHandle<SourceTextModule> &module);

    bool SearchInSModuleManager(JSThread *thread, const CString &recordName);

    void InsertInSModuleManager(JSThread *thread, JSHandle<JSTaggedValue> requireModule,
        JSHandle<SourceTextModule> &moduleRecord);

    void TransferSModule(JSThread *thread);

    bool IsInstaniatedSModule(JSThread *thread, const JSHandle<SourceTextModule> &module);

    JSHandle<JSTaggedValue> GenerateFuncModule(JSThread *thread, const JSPandaFile *jsPandaFile,
        const CString &entryPoint, ClassKind classKind = ClassKind::NON_SENDABLE);

    JSHandle<ModuleNamespace> SModuleNamespaceCreate(JSThread *thread, const JSHandle<JSTaggedValue> &module,
                                                            const JSHandle<TaggedArray> &exports);
private:
    SharedModuleManager() = default;
    ~SharedModuleManager() = default;

    NO_COPY_SEMANTIC(SharedModuleManager);
    NO_MOVE_SEMANTIC(SharedModuleManager);
    JSHandle<SourceTextModule> GetImportedSModule(JSThread *thread, JSTaggedValue referencing);
    JSTaggedValue GetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module, int index) const;

    JSHandle<JSTaggedValue> ResolveSharedImportedModule(JSThread *thread, const CString &referencingModule,
        const JSPandaFile *jsPandaFile, JSRecordInfo *recordInfo);

    JSHandle<JSTaggedValue> ResolveSharedImportedModuleWithMerge(JSThread *thread, const CString &fileName,
        const CString &recordName, const JSPandaFile *jsPandaFile, JSRecordInfo *recordInfo);

    bool SearchInSModuleManagerUnsafe(JSThread *thread, const CString &recordName);

    JSHandle<SourceTextModule> GetSModuleUnsafe(JSThread *thread, const CString &recordName);

    JSHandle<SourceTextModule> GetSModuleUnsafe(JSThread *thread, JSTaggedValue recordName);

    JSHandle<SourceTextModule> GetSModule(JSThread *thread, const CString &recordName);

    JSHandle<SourceTextModule> GetSModule(JSThread *thread, JSTaggedValue recordName);

    static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;
    JSTaggedValue resolvedSharedModules_ {JSTaggedValue::Hole()};
    CMap<CString, StateVisit> sharedModuleMutex_;
    Mutex mutex_;

    friend class SourceTextModule;
    friend class EcmaContext;
    friend class ModuleManager;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_JS_SHARED_MODULE_MANAGER_H
