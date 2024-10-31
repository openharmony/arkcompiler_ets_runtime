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
#ifndef ECMASCRIPT_MODULE_MODULE_RESOLVER_H
#define ECMASCRIPT_MODULE_MODULE_RESOLVER_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module.h"

namespace panda::ecmascript {
class ModuleResolver {
public:
    // 15.2.1.17 Runtime Semantics HostResolveImportedModule ( referencingModule, specifier )
    static JSHandle<JSTaggedValue> HostResolveImportedModule(JSThread *thread,
                                                             const JSHandle<SourceTextModule> &module,
                                                             const JSHandle<JSTaggedValue> &moduleRequest,
                                                             bool executeFromJob = false);
    static CString ReplaceModuleThroughFeature(JSThread *thread, const CString &requestName);
    // Refactoring: From SharedModuleManager::ResolveSharedImportedModule
    static JSHandle<JSTaggedValue> ResolveImportedModuleBundlePack(JSThread *thread,
                                                                   const CString &fileName,
                                                                   bool executeFromJob);
    // Refactoring: From SharedModuleManager::ResolveImportedModuleWithMerge
    static JSHandle<JSTaggedValue> PUBLIC_API ResolveImportedModuleWithMerge(JSThread *thread,
                                                                             const CString &fileName,
                                                                             const CString &recordName,
                                                                             bool executeFromJob);
    // Refactoring: From ModuleManager::ResolveNativeModule
    static JSHandle<JSTaggedValue> ResolveNativeModule(JSThread *thread,
                                                       const CString &moduleRequest,
                                                       const CString &baseFileName,
                                                       ModuleTypes moduleType);
    // Refactoring: From ModuleManager::HostResolveImportedModule
    static JSHandle<JSTaggedValue> HostResolveImportedModuleBundlePack(JSThread *thread,
                                                             const void *buffer,
                                                             size_t size,
                                                             const CString &filename);
    // Refactoring: From ModuleManager::HostResolveImportedModule
    static JSHandle<JSTaggedValue> HostResolveImportedModuleBundlePack(JSThread *thread,
                                                             const CString &referencingModule,
                                                             bool executeFromJob = false);
    // Refactoring: From ModuleManager::HostResolveImportedModuleWithMerge
    static JSHandle<JSTaggedValue> PUBLIC_API HostResolveImportedModuleWithMerge(JSThread *thread,
                                                                                 const CString &referencingModule,
                                                                                 const CString &recordName,
                                                                                 bool executeFromJob = false);
    // Refactoring: From ModuleManager::HostResolveImportedModuleWithMergeForHotReload
    static JSHandle<JSTaggedValue> PUBLIC_API HostResolveImportedModuleWithMergeForHotReload(JSThread *thread,
        const CString &referencingModule,
        const CString &recordName,
        bool executeFromJob = false);
    // Refactoring: From ModuleManager::HostResolveImportedModule
    static JSHandle<JSTaggedValue> HostResolveImportedModuleBundlePack(JSThread *thread,
                                                             const JSPandaFile *jsPandaFile,
                                                             const CString &filename);
private:
    // Refactoring: From SourceTextModule::HostResolveImportedModule
    static JSHandle<JSTaggedValue> HostResolveImportedModuleBundlePack(JSThread *thread,
                                                                       const JSHandle<SourceTextModule> &module,
                                                                       const JSHandle<JSTaggedValue> &moduleRequest,
                                                                       bool executeFromJob = false);
    // Refactoring: From SourceTextModule::HostResolveImportedModuleWithMerge
    static JSHandle<JSTaggedValue> HostResolveImportedModuleWithMerge(JSThread *thread,
                                                                      const JSHandle<SourceTextModule> &module,
                                                                      const JSHandle<JSTaggedValue> &moduleRequest,
                                                                      bool executeFromJob = false);
    // Refactoring: From SharedModuleManager::ResolveSharedImportedModuleWithMerge
    static JSHandle<JSTaggedValue> ResolveSharedImportedModuleWithMerge(JSThread *thread,
                                                                        const CString &fileName,
                                                                        const CString &recordName,
                                                                        const JSPandaFile *jsPandaFile,
                                                                        JSRecordInfo *recordInfo);
    // Refactoring: From ModuleManager::ResolveModule
    static JSHandle<JSTaggedValue> ResolveModuleBundlePack(JSThread *thread,
                                                 const JSPandaFile *jsPandaFile,
                                                 bool executeFromJob = false);
    // Refactoring: From ModuleManager::ResolveModuleWithMerge
    static JSHandle<JSTaggedValue> ResolveModuleWithMerge(JSThread *thread,
                                                          const JSPandaFile *jsPandaFile,
                                                          const CString &recordName,
                                                          bool executeFromJob = false);
    // Refactoring: From ModuleManager::CommonResolveImportedModuleWithMerge
    static JSHandle<JSTaggedValue> CommonResolveImportedModuleWithMerge(JSThread *thread,
                                                                        const CString &moduleFileName,
                                                                        const CString &recordName,
                                                                        bool executeFromJob = false);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_RESOLVER_H