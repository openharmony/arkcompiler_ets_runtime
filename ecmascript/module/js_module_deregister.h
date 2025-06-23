/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MODULE_JS_MODULE_DEREGISTER_H
#define ECMASCRIPT_MODULE_JS_MODULE_DEREGISTER_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {
class ModuleDeregister {
public:
    static inline void InitForDeregisterModule(JSHandle<JSTaggedValue> moduleRecord, const ExecuteTypes &executeType)
    {
        if (IsStaticImport(executeType)) {
            return;
        }
        LoadingTypes moduleLoadingType = LoadingTypes::DYNAMITC_MODULE;
        JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
        module->SetLoadingTypes(moduleLoadingType);
        module->SetRegisterCounts(1);
    }

    static inline void ProcessModuleReference(JSThread *thread, const JSHandle<JSTaggedValue> &nameSpVal)
    {
        JSHandle<ModuleNamespace> nameSp = JSHandle<ModuleNamespace>::Cast(nameSpVal);
        JSHandle<SourceTextModule> moduleRecord(thread, nameSp->GetModule(thread));
        JSTaggedValue weakNameSp = JSTaggedValue(nameSpVal->CreateAndGetWeakRef());
        moduleRecord->SetNamespace(thread, weakNameSp);
    }

    static void FreeModuleRecord(void *env, void *pointer, void *hint);

    static void RemoveModule(JSThread *thread, JSHandle<SourceTextModule> module);

    static void ReviseLoadedModuleCount(JSThread *thread, const CString &moduleName);

    static void IncreaseRegisterCounts(JSThread *thread, JSHandle<SourceTextModule> module,
        std::set<CString> &increaseModule);

    static void DecreaseRegisterCounts(JSThread *thread, JSHandle<SourceTextModule> module,
        std::set<CString> &decreaseModule);

    static bool TryToRemoveSO(JSThread *thread, JSHandle<SourceTextModule> module);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_MODULE_DEREGISTER_H
