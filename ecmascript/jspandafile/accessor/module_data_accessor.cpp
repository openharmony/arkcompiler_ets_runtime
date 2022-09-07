/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/jspandafile/accessor/module_data_accessor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript {
ModuleDataAccessor::ModuleDataAccessor(const panda_file::File &pandaFile, panda_file::File::EntityId moduleDataId)
    : pandaFile_(pandaFile), moduleDataId_(moduleDataId)
{
    auto sp = pandaFile_.GetSpanFromId(moduleDataId);

    auto moduleSp = sp.SubSpan(panda_file::ID_SIZE); // skip literalnum

    numModuleRequests_ = panda_file::helpers::Read<panda_file::ID_SIZE>(&moduleSp);

    for (size_t idx = 0; idx < numModuleRequests_; idx++) {
        auto value = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&moduleSp));
        moduleRequests_.emplace_back(value);
    }

    entryDataSp_ = moduleSp;
}

void ModuleDataAccessor::EnumerateImportEntry(JSThread *thread,
                                              const JSHandle<TaggedArray> &requestModuleArray,
                                              JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConstants = thread->GlobalConstants();
    size_t requestArraySize = requestModuleArray->GetLength();
    auto sp = entryDataSp_;

    auto regularImportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    JSHandle<TaggedArray> regularImportEntries = factory->NewTaggedArray(regularImportNum);
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    for (size_t idx = 0; idx < regularImportNum; idx++) {
        auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto importNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
        StringData sd = pandaFile_.GetStringData(panda_file::File::EntityId(localNameOffset));
        JSHandle<JSTaggedValue> localName(thread,
            factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

        sd = pandaFile_.GetStringData(panda_file::File::EntityId(importNameOffset));
        JSHandle<JSTaggedValue> importName(thread,
            factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

        if (requestArraySize != 0) {
            moduleRequest.Update(requestModuleArray->Get(moduleRequestIdx));
        }
        JSHandle<ImportEntry> importEntry = factory->NewImportEntry(moduleRequest, importName, localName);
        regularImportEntries->Set(thread, idx, importEntry);
    }

    auto namespaceImportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    auto totalSize = namespaceImportNum + regularImportNum;
    JSHandle<TaggedArray> importEntries = TaggedArray::SetCapacity(thread, regularImportEntries, totalSize);
    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    for (size_t idx = 0; idx < namespaceImportNum; idx++) {
        auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
        StringData sd = pandaFile_.GetStringData(panda_file::File::EntityId(localNameOffset));
        JSHandle<JSTaggedValue> localName(thread,
            factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));
        importName.Update(globalConstants->GetHandledStarString());

        if (requestArraySize != 0) {
            moduleRequest.Update(requestModuleArray->Get(moduleRequestIdx));
        }
        JSHandle<ImportEntry> importEntry = factory->NewImportEntry(moduleRequest, importName, localName);
        importEntries->Set(thread, idx + regularImportNum, importEntry);
    }
    if (totalSize) {
        moduleRecord->SetImportEntries(thread, importEntries);
    }
    entryDataSp_ = sp;
}


void ModuleDataAccessor::EnumerateLocalExportEntry(JSThread *thread, JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto sp = entryDataSp_;

    auto localExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    JSHandle<TaggedArray> localExportEntries = factory->NewTaggedArray(localExportNum);
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry();
    for (size_t idx = 0; idx < localExportNum; idx++) {
        auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto exportNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        StringData sd = pandaFile_.GetStringData(panda_file::File::EntityId(localNameOffset));
        JSHandle<JSTaggedValue> localName(thread,
            factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

        sd = pandaFile_.GetStringData(panda_file::File::EntityId(exportNameOffset));
        JSHandle<JSTaggedValue> exportName(thread,
            factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

        localExportEntry = factory->NewLocalExportEntry(exportName, localName);
        localExportEntries->Set(thread, idx, localExportEntry);
    }
    entryDataSp_ = sp;
    if (localExportNum <= SourceTextModule::DEFAULT_ARRAY_CAPACITY) {
        moduleRecord->SetModes(ModuleModes::ARRAYMODE);
    }
    if (localExportNum == SINGLE_MODE_SIZE) {
        moduleRecord->SetLocalExportEntries(thread, localExportEntry);
        return;
    }
    if (localExportNum) {
        moduleRecord->SetLocalExportEntries(thread, localExportEntries);
    }
}

void ModuleDataAccessor::EnumerateIndirectExportEntry(JSThread *thread, const JSHandle<TaggedArray> &requestModuleArray,
                                                      JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConstants = thread->GlobalConstants();
    auto sp = entryDataSp_;
    size_t requestArraySize = requestModuleArray->GetLength();

    auto indirectExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    JSHandle<TaggedArray> indirectExportEntries = factory->NewTaggedArray(indirectExportNum);
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    JSHandle<IndirectExportEntry> indirectExportEntry = factory->NewIndirectExportEntry();
    for (size_t idx = 0; idx < indirectExportNum; idx++) {
        auto exportNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto importNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
        StringData sd = pandaFile_.GetStringData(panda_file::File::EntityId(exportNameOffset));
        JSHandle<JSTaggedValue> exportName(thread,
            factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

        sd = pandaFile_.GetStringData(panda_file::File::EntityId(importNameOffset));
        JSHandle<JSTaggedValue> importName(thread,
            factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

        if (requestArraySize != 0) {
            moduleRequest.Update(requestModuleArray->Get(moduleRequestIdx));
        }
        indirectExportEntry = factory->NewIndirectExportEntry(exportName, moduleRequest, importName);
        indirectExportEntries->Set(thread, idx, indirectExportEntry);
    }
    entryDataSp_ = sp;
    if (indirectExportNum == SINGLE_MODE_SIZE) {
        moduleRecord->SetIndirectExportEntries(thread, indirectExportEntry);
        return;
    }
    if (indirectExportNum) {
        moduleRecord->SetIndirectExportEntries(thread, indirectExportEntries);
    }
}

void ModuleDataAccessor::EnumerateStarExportEntry(JSThread *thread, const JSHandle<TaggedArray> &requestModuleArray,
                                                  JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConstants = thread->GlobalConstants();
    auto sp = entryDataSp_;
    size_t requestArraySize = requestModuleArray->GetLength();

    auto starExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    JSHandle<TaggedArray> starExportEntries = factory->NewTaggedArray(starExportNum);
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    JSHandle<StarExportEntry> starExportEntry = factory->NewStarExportEntry();
    for (size_t idx = 0; idx < starExportNum; idx++) {
        auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
        if (requestArraySize != 0) {
            moduleRequest.Update(requestModuleArray->Get(moduleRequestIdx));
        }

        starExportEntry = factory->NewStarExportEntry(moduleRequest);
        starExportEntries->Set(thread, idx, starExportEntry.GetTaggedValue());
    }
    entryDataSp_ = sp;

    if (starExportNum == SINGLE_MODE_SIZE) {
        moduleRecord->SetStarExportEntries(thread, starExportEntry);
        return;
    }
    if (starExportNum) {
        moduleRecord->SetStarExportEntries(thread, starExportEntries);
    }
}
}  // namespace panda::ecmascript
