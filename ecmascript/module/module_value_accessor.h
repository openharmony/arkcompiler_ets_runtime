/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MODULE_VALUE_ACCESSOR_H
#define ECMASCRIPT_MODULE_VALUE_ACCESSOR_H

#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/stubs/runtime_stubs.h"

namespace panda::ecmascript {
class ModuleValueAccessor {
public:
    static JSTaggedValue GetModuleValueInner(JSThread *thread, int32_t index);
    static JSTaggedValue GetModuleValueInner(JSThread *thread, int32_t index, JSTaggedValue jsFunc);
    static JSTaggedValue GetModuleValueInner(JSThread *thread, int32_t index, JSHandle<JSTaggedValue> curModule);
    static JSTaggedValue GetSendableModuleValueInner(JSThread *thread, int32_t index, JSTaggedValue jsFunc);

    static JSTaggedValue GetModuleValueOuter(JSThread *thread, int32_t index);
    static JSTaggedValue GetModuleValueOuter(JSThread *thread, int32_t index, JSTaggedValue jsFunc);
    static JSTaggedValue GetModuleValueOuter(JSThread *thread, int32_t index, JSHandle<JSTaggedValue> curModule);
    static JSTaggedValue GetLazyModuleValueOuter(JSThread *thread, int32_t index, JSTaggedValue jsFunc);
    static JSTaggedValue GetSendableModuleValueOuter(JSThread *thread, int32_t index, JSTaggedValue jsFunc);
    static JSTaggedValue GetLazySendableModuleValueOuter(JSThread *thread, int32_t index, JSTaggedValue jsFunc);

    static JSTaggedValue GetModuleNamespace(JSThread *thread, int32_t index);
    static JSTaggedValue GetModuleNamespace(JSThread *thread, int32_t index, JSTaggedValue curFunc);

    static JSTaggedValue GetNativeOrCjsModuleValue(JSThread *thread, JSHandle<SourceTextModule> module, int32_t index);
    static JSTaggedValue GetNativeOrCjsModuleValue(JSThread *thread,
                                                   JSHandle<SourceTextModule> module,
                                                   JSTaggedValue bindingName);

    static void StoreModuleValue(JSThread *thread, int32_t index, JSTaggedValue value);
    static void StoreModuleValue(JSThread *thread, int32_t index, JSTaggedValue value, JSTaggedValue jsFunc);
    static JSHandle<JSTaggedValue> GetNativeOrCjsExports(JSThread *thread, JSTaggedValue resolvedModule);
protected:
    struct GetModuleValueFromBindingInfo {
        JSThread *thread;
        JSHandle<SourceTextModule> module;
        JSTaggedValue resolvedBinding;
        int32_t index;
        bool isSendable;
    };
    static JSTaggedValue GetCurrentModule(JSThread *thread);
    static JSTaggedValue GetModuleValue(JSThread *thread, JSHandle<SourceTextModule> module, int32_t index);

    template <bool isLazy>
    static JSTaggedValue GetModuleValueOuterInternal(JSThread *thread,
                                                     int32_t index,
                                                     JSTaggedValue curModule);
    template <bool isLazy>
    static JSTaggedValue GetSendableModuleValueOuterInternal(JSThread *thread,
                                                             int32_t index,
                                                             JSTaggedValue curModule);
    static JSTaggedValue GetModuleNamespaceInternal(JSThread *thread, int32_t index, JSTaggedValue curModule);

    static void StoreModuleValueInternal(JSThread *thread,
                                         JSHandle<SourceTextModule> &curModule,
                                         int32_t index,
                                         JSTaggedValue value);

    template <bool isLazy>
    static JSTaggedValue GetModuleValueFromIndexBinding(const GetModuleValueFromBindingInfo &info);
    template <bool isLazy>
    static JSTaggedValue GetModuleValueFromBinding(const GetModuleValueFromBindingInfo &info);
    template <bool isLazy>
    static JSTaggedValue GetModuleValueFromRecordIndexBinding(const GetModuleValueFromBindingInfo &info);
    template <bool isLazy>
    static JSTaggedValue GetModuleValueFromRecordBinding(const GetModuleValueFromBindingInfo &info);
    static JSTaggedValue UpdateBindingAndGetModuleValue(JSThread *thread,
                                                        JSHandle<SourceTextModule> module,
                                                        JSHandle<SourceTextModule> requiredModule,
                                                        int32_t index,
                                                        JSTaggedValue bindingName);

    template <bool isLazy>
    static void EvaluateModuleIfNeeded(JSThread *thread, JSHandle<SourceTextModule> module);
    static void LogModuleLoadInfo(JSThread *thread,
                                  JSHandle<SourceTextModule> module,
                                  JSHandle<SourceTextModule> requiredModule,
                                  int32_t index,
                                  bool isSendable);

    template <bool isLazy, typename BindingType>
    static JSHandle<SourceTextModule> GetResolvedModule(JSThread *thread,
                                                        JSHandle<SourceTextModule> module,
                                                        JSHandle<BindingType> binding,
                                                        const CString& requestModuleRecordName);

    friend class DeprecatedModuleValueAccessor;
    friend class RuntimeStubs;
};
class DeprecatedModuleValueAccessor {
public:
    static JSTaggedValue GetModuleValueInner(JSThread *thread, JSTaggedValue key);
    static JSTaggedValue GetModuleValueInner(JSThread *thread, JSTaggedValue key, JSTaggedValue jsFunc);

    static JSTaggedValue GetModuleValueOuter(JSThread *thread, JSTaggedValue key);
    static JSTaggedValue GetModuleValueOuter(JSThread *thread, JSTaggedValue key, JSTaggedValue jsFunc);

    static JSTaggedValue GetModuleNamespace(JSThread *thread, JSTaggedValue localName);
    static JSTaggedValue GetModuleNamespace(JSThread *thread, JSTaggedValue localName, JSTaggedValue curFunc);

    static void StoreModuleValue(JSThread *thread, JSTaggedValue key, JSTaggedValue value);
    static void StoreModuleValue(JSThread *thread, JSTaggedValue key, JSTaggedValue value, JSTaggedValue jsFunc);
protected:
    static JSTaggedValue GetModuleValueOuterInternal(JSThread *thread, JSTaggedValue key, JSTaggedValue curModule);
    static JSTaggedValue GetModuleNamespaceInternal(JSThread *thread, JSTaggedValue localName, JSTaggedValue curModule);
    static void StoreModuleValueInternal(JSThread *thread,
                                         JSHandle<SourceTextModule> &curModule,
                                         JSTaggedValue key,
                                         JSTaggedValue value);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_VALUE_ACCESSOR_H
