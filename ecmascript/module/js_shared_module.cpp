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
#include "ecmascript/module/js_shared_module.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> SendableClassModule::GenerateSendableFuncModule(JSThread *thread,
                                                                        const JSHandle<JSTaggedValue> &module)
{
    // esm -> SourceTextModule; cjs or script -> string of recordName
    if (!module->IsSourceTextModule()) {
        ASSERT(module->IsString());
        return module;
    }
    JSHandle<SourceTextModule> currentModule = JSHandle<SourceTextModule>::Cast(module);
    // Only clone module in isolate-heap. 
    if (SourceTextModule::IsModuleInSharedHeap(currentModule)) {
        return module;
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> sModule = factory->NewSModule();
    JSHandle<JSTaggedValue> currentEnvironment(thread, currentModule->GetEnvironment());
    JSHandle<JSTaggedValue> sendableEnvironment = SendableClassModule::CloneModuleEnvironment(thread,
                                                                                              currentEnvironment);
    sModule->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
    sModule->SetEnvironment(thread, sendableEnvironment);
    sModule->SetEcmaModuleFilename(thread, currentModule->GetEcmaModuleFilename());
    sModule->SetEcmaModuleRecordName(thread, currentModule->GetEcmaModuleRecordName());
    return JSHandle<JSTaggedValue>(sModule);
}

JSHandle<JSTaggedValue> SendableClassModule::CloneRecordBinding(JSThread *thread, JSTaggedValue indexBinding)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(indexBinding.GetTaggedObject());
    JSTaggedValue resolvedModule = binding->GetModule();
    JSHandle<EcmaString> record(thread, SourceTextModule::GetModuleName(resolvedModule));
    int32_t index = binding->GetIndex();
    return JSHandle<JSTaggedValue>::Cast(factory->NewSResolvedRecordBindingRecord(record, index));
}

JSHandle<JSTaggedValue> SendableClassModule::CloneModuleEnvironment(JSThread *thread,
                                                                    const JSHandle<JSTaggedValue> &moduleEnvironment)
{
    if (moduleEnvironment->IsUndefined()) {
        return moduleEnvironment;
    }
    JSHandle<TaggedArray> currentEnvironment(moduleEnvironment);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int enumKeys = currentEnvironment->GetLength();
    JSHandle<TaggedArray> sendableEnvironment = factory->NewSDictionaryArray(enumKeys);
    for (int idx = 0; idx < enumKeys; idx++) {
        JSTaggedValue key = currentEnvironment->Get(idx);
        // [[TODO::DaiHN will consider ResolvedBinding]]
        if (key.IsResolvedIndexBinding()) {
            JSHandle<JSTaggedValue> recordBinding = SendableClassModule::CloneRecordBinding(thread, key);
            sendableEnvironment->Set(thread, idx, recordBinding);
        }
        continue;
    }
    return JSHandle<JSTaggedValue>(sendableEnvironment);
}
} // namespace panda::ecmascript
