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
#include "ecmascript/global_env.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/shared_objects/js_shared_array.h"

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
    sModule->SetEcmaModuleFilenameString(currentModule->GetEcmaModuleFilenameString());
    sModule->SetEcmaModuleRecordNameString(currentModule->GetEcmaModuleRecordNameString());
    sModule->SetSendableEnv(thread, JSTaggedValue::Undefined());
    return JSHandle<JSTaggedValue>(sModule);
}

JSHandle<JSTaggedValue> SendableClassModule::CloneRecordIndexBinding(JSThread *thread, JSTaggedValue indexBinding)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ResolvedIndexBinding> binding(thread, indexBinding);
    JSHandle<SourceTextModule> resolvedModule(thread, binding->GetModule());
    if (SourceTextModule::IsSharedModule((resolvedModule))) {
        return JSHandle<JSTaggedValue>::Cast(
            factory->NewSResolvedIndexBindingRecord(resolvedModule, binding->GetIndex()));
    }

    CString moduleName = SourceTextModule::GetModuleName(resolvedModule.GetTaggedValue());
    JSHandle<EcmaString> record = thread->GetEcmaVM()->GetFactory()->NewFromUtf8(moduleName);
    JSHandle<EcmaString> fileName = factory->NewFromUtf8(resolvedModule->GetEcmaModuleFilenameString());
    int32_t index = binding->GetIndex();
    return JSHandle<JSTaggedValue>::Cast(factory->NewSResolvedRecordIndexBindingRecord(record, fileName, index));
}

JSHandle<JSTaggedValue> SendableClassModule::CloneRecordNameBinding(JSThread *thread, JSTaggedValue binding)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ResolvedBinding *resolvedBinding = ResolvedBinding::Cast(binding.GetTaggedObject());
    JSHandle<SourceTextModule> resolvedModule(thread, resolvedBinding->GetModule());
    if (SourceTextModule::IsSharedModule((resolvedModule))) {
        JSHandle<JSTaggedValue> bindingName(thread, resolvedBinding->GetBindingName());
        return JSHandle<JSTaggedValue>::Cast(
            factory->NewSResolvedBindingRecord(resolvedModule, bindingName));
    }

    CString moduleName = SourceTextModule::GetModuleName(resolvedModule.GetTaggedValue());
    JSHandle<EcmaString> record = thread->GetEcmaVM()->GetFactory()->NewFromUtf8(moduleName);
    JSHandle<JSTaggedValue> bindingName(thread, resolvedBinding->GetBindingName());
    return JSHandle<JSTaggedValue>::Cast(factory->NewSResolvedRecordBindingRecord(record, bindingName));
}

JSHandle<TaggedArray> SendableClassModule::CloneModuleEnvironment(JSThread *thread,
                                                                  const JSHandle<JSTaggedValue> &moduleEnvironment)
{
    if (moduleEnvironment->IsUndefined()) {
        return JSHandle<TaggedArray>(moduleEnvironment);
    }
    JSHandle<TaggedArray> currentEnvironment(moduleEnvironment);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int enumKeys = static_cast<int>(currentEnvironment->GetLength());
    JSHandle<TaggedArray> sendableEnvironment = factory->NewSDictionaryArray(enumKeys);
    for (int idx = 0; idx < enumKeys; idx++) {
        JSTaggedValue key = currentEnvironment->Get(idx);
        if (key.IsRecord()) {
            JSType type = key.GetTaggedObject()->GetClass()->GetObjectType();
            switch (type) {
                case JSType::RESOLVEDBINDING_RECORD: {
                    JSHandle<JSTaggedValue> recordBinding = SendableClassModule::CloneRecordNameBinding(thread, key);
                    sendableEnvironment->Set(thread, idx, recordBinding);
                    break;
                }
                case JSType::RESOLVEDINDEXBINDING_RECORD: {
                    JSHandle<JSTaggedValue> recordBinding = SendableClassModule::CloneRecordIndexBinding(thread, key);
                    sendableEnvironment->Set(thread, idx, recordBinding);
                    break;
                }
                case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
                    break;
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
                                                              const CString &descriptor, const CString &moduleFilename,
                                                              JSRecordInfo *recordInfo)
{
    int moduleIdx = jsPandaFile->GetModuleRecordIdx(descriptor);
    ASSERT(jsPandaFile->IsNewVersion() && (moduleIdx != -1)); // new pandafile version use new literal offset mechanism
    panda_file::File::EntityId moduleId = panda_file::File::EntityId(static_cast<uint32_t>(moduleIdx));

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSSourceTextModule();
    moduleRecord->SetSharedType(SharedTypes::SHARED_MODULE);
    ModuleDataExtractor::ExtractModuleDatas(thread, jsPandaFile, moduleId, moduleRecord, recordInfo);

    bool hasTLA = jsPandaFile->GetHasTopLevelAwait(descriptor);
    moduleRecord->SetHasTLA(hasTLA);
    moduleRecord->SetEcmaModuleFilenameString(moduleFilename);
    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::ECMA_MODULE);
    moduleRecord->SetIsNewBcVersion(true);
    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

JSHandle<TaggedArray> JSSharedModule::GenerateSharedExports(JSThread *thread, const JSHandle<TaggedArray> &oldExports)
{
    uint32_t newLength = oldExports->GetLength();
    if (newLength == 0) {
        LOG_FULL(INFO) << "Empty exports in moduleNamespace";
    }
    JSHandle<TaggedArray> newArray = thread->GetEcmaVM()->GetFactory()->NewSTaggedArray(newLength);
    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = oldExports->Get(i);
        ASSERT(value.IsString());
        newArray->Set(thread, i, value);
    }
    return newArray;
}

JSHandle<JSTaggedValue> JSSharedModule::CreateSharedSortedExports(JSThread *thread,
    const JSHandle<TaggedArray> &oldExports)
{
    auto globalConst = thread->GlobalConstants();
    JSHandle<TaggedArray> exports = GenerateSharedExports(thread, oldExports);
    // 7. Let sortedExports be a new List containing the same values as the list exports where the values
    // are ordered as if an Array of the same values had been sorted using
    // Array.prototype.sort using undefined as comparefn.
    JSHandle<JSSharedArray> exportsArray = JSSharedArray::CreateArrayFromList(thread, exports);
    JSHandle<JSTaggedValue> sortedExports = JSHandle<JSTaggedValue>::Cast(exportsArray);
    JSHandle<JSTaggedValue> fn = globalConst->GetHandledUndefined();
    JSSharedArray::Sort(thread, sortedExports, fn);
    return sortedExports;
}

JSHandle<ModuleNamespace> JSSharedModule::SModuleNamespaceCreate(JSThread *thread,
                                                                 const JSHandle<JSTaggedValue> &module,
                                                                 const JSHandle<TaggedArray> &exports)
{
    auto globalConst = thread->GlobalConstants();
    // 2. Assert: module.[[Namespace]] is undefined.
    // 3. Assert: exports is a List of String values.
    // 4. Let M be a newly created object.
    // 5. Set M's essential internal methods to the definitions specified in 9.4.6.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ModuleNamespace> mNp = factory->NewSModuleNamespace();
    // 6. Set M.[[Module]] to module.
    mNp->SetModule(thread, module);
    JSHandle<JSTaggedValue> sortedExports = JSSharedModule::CreateSharedSortedExports(thread, exports);

    // 8. Set M.[[Exports]] to sortedExports.
    mNp->SetExports(thread, sortedExports);
    // 9. Create own properties of M corresponding to the definitions in 26.3.

    JSHandle<JSTaggedValue> toStringTag = thread->GetEcmaVM()->GetGlobalEnv()->GetToStringTagSymbol();
    JSHandle<JSTaggedValue> moduleString = globalConst->GetHandledModuleString();
    PropertyDescriptor des(thread, moduleString, false, false, false);
    JSHandle<JSObject> mNpObj = JSHandle<JSObject>::Cast(mNp);
    JSObject::DefineOwnProperty(thread, mNpObj, toStringTag, des);
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    SourceTextModule::Cast(moduleRecord.GetTaggedValue().GetTaggedObject())->SetNamespace(thread,
                                                                                          mNp.GetTaggedValue());
    // 10. Set module.[[Namespace]] to M.
    mNp->GetJSHClass()->SetExtensible(false);
    return mNp;
}
} // namespace panda::ecmascript
