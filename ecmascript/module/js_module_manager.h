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

namespace panda::ecmascript {
class ModuleManager {
public:
    explicit ModuleManager(EcmaVM *vm);
    ~ModuleManager() = default;

    JSTaggedValue GetModuleValueInner(JSTaggedValue key);
    JSTaggedValue GetModuleValueInner(JSTaggedValue key, JSTaggedValue jsFunc);
    JSTaggedValue GetModuleValueOutter(JSTaggedValue key);
    JSTaggedValue GetModuleValueOutter(JSTaggedValue key, JSTaggedValue jsFunc);
    void StoreModuleValue(JSTaggedValue key, JSTaggedValue value);
    void StoreModuleValue(JSTaggedValue key, JSTaggedValue value, JSTaggedValue jsFunc);

    JSHandle<SourceTextModule> HostGetImportedModule(const CString &referencingModule);
    JSHandle<SourceTextModule> HostGetImportedModule(JSTaggedValue referencing);
    bool IsImportedModuleLoaded(JSTaggedValue referencing);

    JSHandle<SourceTextModule> HostResolveImportedModule(const void *buffer, size_t size, const CString &filename);
    JSHandle<SourceTextModule> HostResolveImportedModule(std::string &baseFilename, std::string &moduleFilename);
    JSHandle<SourceTextModule> HostResolveImportedModule(const CString &referencingModule);
    JSHandle<SourceTextModule> HostResolveImportedModuleWithMerge(const CString &referencingModule,
                                                                  const CString &recordName);

    JSTaggedValue GetModuleNamespace(JSTaggedValue localName);
    JSTaggedValue GetModuleNamespace(JSTaggedValue localName, JSTaggedValue currentFunc);
    JSTaggedValue GetCurrentModule();
    JSTaggedValue GetModuleNamespaceInternal(JSTaggedValue localName, JSTaggedValue currentModule);

    void AddResolveImportedModule(const JSPandaFile *jsPandaFile, const CString &referencingModule);
    void Iterate(const RootVisitor &v);
    bool GetCurrentMode() const
    {
        return isExecuteBuffer_;
    }
    void SetExecuteMode(bool mode)
    {
        isExecuteBuffer_ = mode;
    }
    void ConcatFileName(std::string &dirPath, std::string &requestPath, std::string &fileName);

    // use for AOT PassManager
    PUBLIC_API CString ResolveModuleFileName(const CString &fileName);

private:
    NO_COPY_SEMANTIC(ModuleManager);
    NO_MOVE_SEMANTIC(ModuleManager);

    JSTaggedValue GetModuleValueOutterInternal(JSTaggedValue key, JSTaggedValue currentModule);
    void StoreModuleValueInternal(JSHandle<SourceTextModule> &currentModule,
                                  JSTaggedValue key, JSTaggedValue value);

    JSHandle<SourceTextModule> ResolveModule(JSThread *thread, const JSPandaFile *jsPandaFile);
    JSHandle<SourceTextModule> ResolveModuleWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                      const CString &recodeName);

    static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;

    EcmaVM *vm_ {nullptr};
    JSTaggedValue resolvedModules_ {JSTaggedValue::Hole()};
    bool isExecuteBuffer_ = false;

    friend class EcmaVM;
    friend class QuickFixLoader;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_JS_MODULE_MANAGER_H
