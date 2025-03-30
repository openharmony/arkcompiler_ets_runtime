/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/module/module_data_extractor.h"

#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/module/accessor/module_data_accessor.h"

namespace panda::ecmascript {
using StringData = panda_file::StringData;
using BuiltinsJson = builtins::BuiltinsJson;
using JSRecordInfo = ecmascript::JSPandaFile::JSRecordInfo;

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                         const CString &descriptor, const CString &moduleFilename,
                                                         JSRecordInfo *recordInfo)
{
    int moduleIdx = jsPandaFile->GetModuleRecordIdx(descriptor);
    ASSERT(moduleIdx != -1);
    panda_file::File::EntityId moduleId;
    if (jsPandaFile->IsNewVersion()) {  // new pandafile version use new literal offset mechanism
        moduleId = panda_file::File::EntityId(static_cast<uint32_t>(moduleIdx));
    } else {
        panda_file::LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
        moduleId = lda.GetLiteralArrayId(static_cast<size_t>(moduleIdx));
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();
    ExtractModuleDatas(thread, jsPandaFile, moduleId, moduleRecord, recordInfo);

    bool hasTLA = recordInfo->hasTopLevelAwait;
    moduleRecord->SetHasTLA(hasTLA);

    moduleRecord->SetEcmaModuleFilenameString(moduleFilename);

    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::ECMA_MODULE);
    moduleRecord->SetIsNewBcVersion(jsPandaFile->IsNewVersion());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

void ModuleDataExtractor::ExtractModuleDatas(JSThread *thread, const JSPandaFile *jsPandaFile,
                                             panda_file::File::EntityId moduleId,
                                             JSHandle<SourceTextModule> &moduleRecord,
                                             [[maybe_unused]]JSRecordInfo *recordInfo)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ModuleDataAccessor mda(jsPandaFile, moduleId);
    const std::vector<uint32_t> &moduleRequests = mda.getModuleRequests();
    size_t len = moduleRequests.size();
    JSHandle<TaggedArray> moduleRequestArray;
    JSHandle<TaggedArray> requestModuleArray(thread->GlobalConstants()->GetHandledUndefined());
    bool isShared = SourceTextModule::IsSharedModule(moduleRecord);
    if (isShared) {
        moduleRequestArray = factory->NewSTaggedArray(len, JSTaggedValue::Hole(), MemSpaceType::SHARED_OLD_SPACE);
    } else {
        moduleRequestArray = factory->NewTaggedArray(len);
        requestModuleArray = factory->NewTaggedArray(len);
    }

    for (size_t idx = 0; idx < len; idx++) {
        StringData sd = jsPandaFile->GetStringData(panda_file::File::EntityId(moduleRequests[idx]));
        JSTaggedValue value(factory->GetRawStringFromStringTable(sd));
        moduleRequestArray->Set(thread, idx, value);
    }
    if (len > 0) {
        moduleRecord->SetModuleRequests(thread, moduleRequestArray);
        // For .[RequestedModules], normal module will later replace by sourceTextModule
        moduleRecord->SetRequestedModules(thread, requestModuleArray);
        if (isShared) {
            moduleRecord->SetRequestedModules(thread, moduleRequestArray);
        }
    }

    uint32_t lazyImportIdx = recordInfo->lazyImportIdx;
    if (lazyImportIdx != 0) {
        bool *lazyImportFlags =
            ModuleLazyImportFlagAccessor(jsPandaFile, panda_file::File::EntityId(lazyImportIdx));
        moduleRecord->SetLazyImportArray(lazyImportFlags); // set module Lazy Import flag
    }
    // note the order can't change
    mda.EnumerateImportEntry(thread, moduleRecord);
    mda.EnumerateLocalExportEntry(thread, moduleRecord);
    mda.EnumerateIndirectExportEntry(thread, moduleRecord);
    mda.EnumerateStarExportEntry(thread, moduleRecord);
}

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseCjsModule(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    const CString &descriptor = jsPandaFile->GetJSPandaFileDesc();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();

    moduleRecord->SetEcmaModuleFilenameString(descriptor);

    JSHandle<JSTaggedValue> defaultName = thread->GlobalConstants()->GetHandledDefaultString();
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(defaultName,
        defaultName, LocalExportEntry::LOCAL_DEFAULT_INDEX, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, moduleRecord, localExportEntry, 0, 1); // 1 means len
    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::CJS_MODULE);
    moduleRecord->SetIsNewBcVersion(jsPandaFile->IsNewVersion());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseJsonModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                             const CString &moduleFilename, const CString &recordName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();

    JSHandle<JSTaggedValue> defaultName = thread->GlobalConstants()->GetHandledDefaultString();
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(defaultName,
        defaultName, LocalExportEntry::LOCAL_DEFAULT_INDEX, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, moduleRecord, localExportEntry, 0, 1); // 1 means len
    JSTaggedValue jsonData = JsonParse(thread, jsPandaFile, recordName);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    SourceTextModule::StoreModuleValue(thread, moduleRecord, 0, JSHandle<JSTaggedValue>(thread, jsonData)); // index = 0

    moduleRecord->SetEcmaModuleFilenameString(moduleFilename);

    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::JSON_MODULE);
    moduleRecord->SetIsNewBcVersion(jsPandaFile->IsNewVersion());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseNativeModule(JSThread *thread, const CString &moduleRequestName,
                                                               const CString &baseFileName, ModuleTypes moduleType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();

    // set moduleRecordName as non-undefined to distinguish between merge and non-merge mode
    moduleRecord->SetEcmaModuleRecordNameString(moduleRequestName);
    moduleRecord->SetEcmaModuleFilenameString(baseFileName);
    JSHandle<JSTaggedValue> defaultName = thread->GlobalConstants()->GetHandledDefaultString();
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(defaultName,
        defaultName, LocalExportEntry::LOCAL_DEFAULT_INDEX, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, moduleRecord, localExportEntry, 0, 1);
    moduleRecord->SetTypes(moduleType);
    moduleRecord->SetIsNewBcVersion(true);
    moduleRecord->SetStatus(ModuleStatus::INSTANTIATED);
    SourceTextModule::StoreModuleValue(thread, moduleRecord, 0, thread->GlobalConstants()->GetHandledUndefined());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

JSTaggedValue ModuleDataExtractor::JsonParse(JSThread *thread, const JSPandaFile *jsPandaFile, CString entryPoint)
{
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(
            thread, undefined, undefined, undefined, 1); // 1 : argument numbers
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSRecordInfo *recordInfo = jsPandaFile->CheckAndGetRecordInfo(entryPoint);
    ASSERT(recordInfo != nullptr);
    StringData sd = jsPandaFile->GetStringData(EntityId(recordInfo->jsonStringId));
    JSTaggedValue value(thread->GetEcmaVM()->GetFactory()->GetRawStringFromStringTable(sd));
    info->SetCallArg(value);
    return BuiltinsJson::Parse(info);
}

bool* ModuleDataExtractor::ModuleLazyImportFlagAccessor(const JSPandaFile *pandaFile, EntityId lazyImportFlagId)
{
    auto &pf = *pandaFile->GetPandaFile();
    auto sp = pf.GetSpanFromId(lazyImportFlagId);

    uint32_t numLazyImportFlags = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    bool *lazyImportArray = new bool[numLazyImportFlags]();
    for (size_t idx = 0; idx < numLazyImportFlags; idx++) {
        uint32_t value = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint8_t)>(&sp));
        lazyImportArray[idx] = value > 0 ? 1 : 0;
    }
    return lazyImportArray;
}
}  // namespace panda::ecmascript
