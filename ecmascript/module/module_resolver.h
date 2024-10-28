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

namespace panda::ecmascript {
class ModuleResolver {
public:
    // 15.2.1.17 Runtime Semantics HostResolveImportedModule ( referencingModule, specifier )
    static JSHandle<JSTaggedValue> HostResolveImportedModule(JSThread *thread,
                                                             const JSHandle<SourceTextModule> &module,
                                                             const JSHandle<JSTaggedValue> &moduleRequest,
                                                             bool executeFromJob = false);
    static CString ReplaceModuleThroughFeature(JSThread *thread, const CString &requestName);
private:
    static JSHandle<JSTaggedValue> HostResolveImportedModuleBundlePack(JSThread *thread,
                                                             const JSHandle<SourceTextModule> &module,
                                                             const JSHandle<JSTaggedValue> &moduleRequest,
                                                             bool executeFromJob = false);
    static JSHandle<JSTaggedValue> HostResolveImportedModuleWithMerge(JSThread *thread,
                                                                      const JSHandle<SourceTextModule> &module,
                                                                      const JSHandle<JSTaggedValue> &moduleRequest,
                                                                      bool executeFromJob = false);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_RESOLVER_H