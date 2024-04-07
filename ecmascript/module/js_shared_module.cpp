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
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/tagged_array-inl.h"
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
    JSHandle<SourceTextModule> sModule = factory->NewSSourceTextModule();
    JSHandle<JSTaggedValue> currentEnvironment(thread, currentModule->GetEnvironment());
    JSHandle<TaggedArray> sendableEnvironment = SendableClassModule::CloneModuleEnvironment(thread,
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
    JSHandle<SourceTextModule> resolvedModule(thread, binding->GetModule());
    if (SourceTextModule::IsSharedModule((resolvedModule))) {
        return JSHandle<JSTaggedValue>::Cast(
            factory->NewSResolvedIndexBindingRecord(resolvedModule, binding->GetIndex()));
    }

    JSHandle<EcmaString> record(thread, SourceTextModule::GetModuleName(resolvedModule.GetTaggedValue()));
    int32_t index = binding->GetIndex();
    return JSHandle<JSTaggedValue>::Cast(factory->NewSResolvedRecordBindingRecord(record, index));
}

JSHandle<TaggedArray> SendableClassModule::CloneModuleEnvironment(JSThread *thread,
                                                                  const JSHandle<JSTaggedValue> &moduleEnvironment)
{
    if (moduleEnvironment->IsUndefined()) {
        return JSHandle<TaggedArray>(moduleEnvironment);
    }
    JSHandle<TaggedArray> currentEnvironment(moduleEnvironment);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int enumKeys = currentEnvironment->GetLength();
    JSHandle<TaggedArray> sendableEnvironment = factory->NewSDictionaryArray(enumKeys);
    for (int idx = 0; idx < enumKeys; idx++) {
        JSTaggedValue key = currentEnvironment->Get(idx);
        if (key.IsRecord()) {
            JSType type = key.GetTaggedObject()->GetClass()->GetObjectType();
            switch (type) {
                case JSType::RESOLVEDBINDING_RECORD:
                    LOG_FULL(ERROR) << "recordBinding appears in shared-module";
                    break;
                case JSType::RESOLVEDINDEXBINDING_RECORD: {
                    JSHandle<JSTaggedValue> recordBinding = SendableClassModule::CloneRecordBinding(thread, key);
                    sendableEnvironment->Set(thread, idx, recordBinding);
                    break;
                }
                case JSType::RESOLVEDRECORDBINDING_RECORD:
                    break;
                default:
                    LOG_FULL(FATAL) << "UNREACHABLE";
            }
        }
        continue;
    }
    return sendableEnvironment;
}

JSHandle<TaggedArray> JSSharedModule::CloneEnvForSModule(JSThread *thread, const JSHandle<SourceTextModule> &module,
    JSHandle<TaggedArray> &envRec)
{
    if (!SourceTextModule::IsSharedModule(module)) {
        return envRec;
    }
    return SendableClassModule::CloneModuleEnvironment(thread, JSHandle<JSTaggedValue>(envRec));
}

JSHandle<JSTaggedValue> SharedModuleHelper::ParseSharedModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                              const CString &descriptor, const CString &moduleFilename)
{
    int moduleIdx = jsPandaFile->GetModuleRecordIdx(descriptor);
    ASSERT(jsPandaFile->IsNewVersion() && (moduleIdx != -1)); // new pandafile version use new literal offset mechanism
    panda_file::File::EntityId moduleId = panda_file::File::EntityId(static_cast<uint32_t>(moduleIdx));

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSSourceTextModule();
    moduleRecord->SetSharedType(SharedTypes::SHARED_MODULE);
    ModuleDataExtractor::ExtractModuleDatas(thread, jsPandaFile, moduleId, moduleRecord);

    bool hasTLA = jsPandaFile->GetHasTopLevelAwait(descriptor);
    moduleRecord->SetHasTLA(hasTLA);
    JSHandle<EcmaString> ecmaModuleFilename = factory->NewFromUtf8(moduleFilename);
    moduleRecord->SetEcmaModuleFilename(thread, ecmaModuleFilename);
    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::ECMA_MODULE);
    moduleRecord->SetIsNewBcVersion(true);
    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}
} // namespace panda::ecmascript
