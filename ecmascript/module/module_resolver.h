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
                                                             const ExecuteTypes& executeType = ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> PUBLIC_API HostResolveImportedModule(JSThread *thread,
        const CString &fileName,
        const CString &recordName = JSPandaFile::ENTRY_MAIN_FUNCTION,
        const JSPandaFile *jsPandaFile = nullptr,
        const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> PUBLIC_API HostResolveImportedModule(JSThread *thread,
        const CString &fileName,
        const CString &recordName,
        const void *buffer,
        size_t size,
        const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> PUBLIC_API HostResolveImportedModuleForHotReload(JSThread *thread,
        const CString &referencingModule,
        const CString &recordName,
        const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> ResolveNativeModule(JSThread *thread,
        const CString &moduleRequest,
        const CString &baseFileName,
        ModuleTypes moduleType);

    static void ReplaceModuleThroughFeature(JSThread *thread, CString &requestName);

private:
    static JSHandle<JSTaggedValue> HostResolveImportedModuleBundlePack(JSThread *thread,
        const JSHandle<SourceTextModule> &module,
        const JSHandle<JSTaggedValue> &moduleRequest,
        const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> HostResolveImportedModuleWithMerge(JSThread *thread,
        const JSHandle<SourceTextModule> &module,
        const JSHandle<JSTaggedValue> &moduleRequest,
        const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> HostResolveImportedModuleBundlePackBuffer(JSThread *thread,
                                                                             const CString &referencingModule,
                                                                             const JSPandaFile *jsPandaFile,
                                                                             const ExecuteTypes &executeType =
                                                                             ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> HostResolveImportedModuleBundlePack(JSThread *thread,
        const CString &referencingModule,
        const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> HostResolveImportedModuleWithMerge(JSThread *thread,
        const CString &referencingModule,
        const CString &recordName,
        const JSPandaFile *jsPandaFile = nullptr,
        const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    static JSHandle<JSTaggedValue> ResolveSharedImportedModuleWithMerge(JSThread *thread,
                                                                        const CString &fileName,
                                                                        const CString &recordName,
                                                                        const JSPandaFile *jsPandaFile,
                                                                        JSRecordInfo *recordInfo);
    static JSHandle<JSTaggedValue> ResolveModuleBundlePack(JSThread *thread,
                                                           const JSPandaFile *jsPandaFile,
                                                           const ExecuteTypes &executeType = ExecuteTypes::STATIC);
    static JSHandle<JSTaggedValue> ResolveModuleWithMerge(JSThread *thread,
                                                          const JSPandaFile *jsPandaFile,
                                                          const CString &recordName,
                                                          JSRecordInfo *recordInfo,
                                                          const ExecuteTypes &executeType = ExecuteTypes::STATIC);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_RESOLVER_H