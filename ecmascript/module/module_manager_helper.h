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

#ifndef ECMASCRIPT_MODULE_MANAGER_HELPER_H
#define ECMASCRIPT_MODULE_MANAGER_HELPER_H

#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {

class ModuleManagerHelper {
public:
    static JSTaggedValue PUBLIC_API GetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module, int index);

    static JSTaggedValue GetModuleValue(JSThread *thread,
                                        JSHandle<SourceTextModule> module,
                                        JSTaggedValue bindingName);

    static JSTaggedValue GetNativeOrCjsModuleValue(JSThread *thread,
                                                   JSTaggedValue resolvedModule,
                                                   int32_t index);

    static JSHandle<JSTaggedValue> GetNativeOrCjsExports(JSThread *thread,
                                                         JSTaggedValue resolvedModule);

    static JSTaggedValue PUBLIC_API GetModuleValueFromIndexBinding(JSThread *thread,
                                                                   JSHandle<SourceTextModule> module,
                                                                   JSTaggedValue resolvedBinding);

    static JSTaggedValue PUBLIC_API GetModuleValueFromRecordBinding(JSThread *thread,
                                                                    JSHandle<SourceTextModule> module,
                                                                    JSTaggedValue resolvedBinding);
    
    static JSHandle<SourceTextModule> GetResolvedRecordIndexBindingModule(
        JSThread *thread, JSHandle<SourceTextModule> module, JSHandle<ResolvedRecordIndexBinding> binding);

    static JSHandle<SourceTextModule> GetResolvedRecordBindingModule(
        JSThread *thread, JSHandle<SourceTextModule> module, JSHandle<ResolvedRecordBinding> binding);

    static JSTaggedValue GetLazyModuleValueFromIndexBinding(JSThread *thread,
                                                            JSHandle<SourceTextModule> module,
                                                            JSTaggedValue resolvedBinding);

    static JSTaggedValue GetLazyModuleValueFromRecordBinding(JSThread *thread,
                                                             JSHandle<SourceTextModule> module,
                                                             JSTaggedValue resolvedBinding);

    static JSTaggedValue UpdateBindingAndGetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module,
        JSHandle<SourceTextModule> requiredModule, int32_t index, JSTaggedValue bindingName);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_SHARED_MODULE_H
