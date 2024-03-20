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
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
class SharedModuleManager {
public:
    static SharedModuleManager *GetInstance();

    void Initialize(const EcmaVM *vm)
    {
        resolvedSharedModules_ = NameDictionary::CreateInSharedHeap(vm->GetJSThread(),
            DEAULT_DICTIONART_CAPACITY).GetTaggedValue();
    }
    JSTaggedValue GetSendableModuleValue(JSThread *thread, int32_t index, JSTaggedValue jsFunc) const;
    JSTaggedValue GetSendableModuleValueImpl(JSThread *thread, int32_t index, JSTaggedValue currentModule) const;

private:
    SharedModuleManager() = default;
    ~SharedModuleManager() = default;

    NO_COPY_SEMANTIC(SharedModuleManager);
    NO_MOVE_SEMANTIC(SharedModuleManager);
    JSHandle<SourceTextModule> GetImportedSModule(JSThread *thread, JSTaggedValue referencing);

    static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;

    JSTaggedValue resolvedSharedModules_ {JSTaggedValue::Hole()};

    friend class EcmaVM;
    friend class PatchLoader;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_JS_SHARED_MODULE_MANAGER_H
