/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#include "ecmascript/module/accessor/module_data_accessor.h"

#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/shared_objects/js_shared_array.h"

namespace panda::ecmascript {
ModuleDataAccessor::ModuleDataAccessor(const JSPandaFile *pandaFile, EntityId moduleDataId)
    : pandaFile_(pandaFile), moduleDataId_(moduleDataId)
{
    auto &pf = *pandaFile_->GetPandaFile();
    auto sp = pf.GetSpanFromId(moduleDataId);

    auto moduleSp = sp.SubSpan(panda_file::ID_SIZE); // skip literalnum

    numModuleRequests_ = panda_file::helpers::Read<panda_file::ID_SIZE>(&moduleSp);

    for (size_t idx = 0; idx < numModuleRequests_; idx++) {
        auto value = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&moduleSp));
        moduleRequests_.emplace_back(value);
    }

    entryDataSp_ = moduleSp;
}

JSHandle<TaggedArray> ModuleDataAccessor::CreatEntries(JSThread *thread, uint32_t regularImportNum,
                                                       SharedTypes sharedType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> regularImportEntries;
    if (sharedType == SharedTypes::SHARED_MODULE) {
        regularImportEntries = factory->NewSTaggedArray(regularImportNum, JSTaggedValue::Hole(),
            MemSpaceType::SHARED_OLD_SPACE);
    } else {
        regularImportEntries = factory->NewTaggedArray(regularImportNum);
    }
    return regularImportEntries;
}

void ModuleDataAccessor::EnumerateImportEntry(JSThread *thread, JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConstants = thread->GlobalConstants();
    auto sp = entryDataSp_;
    SharedTypes sharedType = moduleRecord->GetSharedType();
    uint32_t regularImportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    JSHandle<TaggedArray> regularImportEntries = CreatEntries(thread, regularImportNum, sharedType);

    JSMutableHandle<JSTaggedValue> importName(thread, globalConstants->GetUndefined());
    JSMutableHandle<JSTaggedValue> localName(thread, globalConstants->GetUndefined());
    uint32_t moduleRequestIdx = 0;

    for (uint32_t idx = 0; idx < regularImportNum; idx++) {
        ReadRegularImportEntry(&sp, factory, importName, localName, moduleRequestIdx);
        JSHandle<ImportEntry> importEntry = factory->NewImportEntry(moduleRequestIdx, importName, localName,
                                                                    sharedType);
        regularImportEntries->Set(thread, idx, importEntry);
    }
    uint32_t namespaceImportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    uint32_t totalSize = regularImportNum + namespaceImportNum;
    if (totalSize == 0) {
        entryDataSp_ = sp;
        return;
    }
    JSHandle<TaggedArray> importEntries;
    if (sharedType == SharedTypes::SHARED_MODULE) {
        importEntries = JSSharedArray::SetCapacity(thread, regularImportEntries, totalSize);
    } else {
        importEntries = TaggedArray::SetCapacity(thread, regularImportEntries, totalSize);
    }

    importName.Update(globalConstants->GetHandledStarString());

    for (uint32_t idx = regularImportNum; idx < totalSize; idx++) {
        ReadNamespaceImportEntry(&sp, factory, localName, moduleRequestIdx);
        JSHandle<ImportEntry> importEntry = factory->NewImportEntry(moduleRequestIdx, importName, localName,
                                                                    sharedType);
        importEntries->Set(thread, idx, importEntry);
    }
    entryDataSp_ = sp;
    moduleRecord->SetImportEntries(thread, importEntries);
}

void ModuleDataAccessor::ReadRegularImportEntry(Span<const uint8_t> *sp, ObjectFactory *factory,
                                                JSMutableHandle<JSTaggedValue> &importName,
                                                JSMutableHandle<JSTaggedValue> &localName,
                                                uint32_t &moduleRequestIdx)
{
    auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(sp));
    auto importNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(sp));
    moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(sp));
    auto sd = pandaFile_->GetStringData(panda_file::File::EntityId(localNameOffset));
    localName.Update(JSTaggedValue(factory->GetRawStringFromStringTable(sd)));

    sd = pandaFile_->GetStringData(panda_file::File::EntityId(importNameOffset));
    importName.Update(JSTaggedValue(factory->GetRawStringFromStringTable(sd)));
}

void ModuleDataAccessor::ReadNamespaceImportEntry(Span<const uint8_t> *sp, ObjectFactory *factory,
                                                  JSMutableHandle<JSTaggedValue> &localName,
                                                  uint32_t &moduleRequestIdx)
{
    auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(sp));
    moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(sp));
    auto sd = pandaFile_->GetStringData(panda_file::File::EntityId(localNameOffset));
    localName.Update(JSTaggedValue(factory->GetRawStringFromStringTable(sd)));
}

void ModuleDataAccessor::EnumerateLocalExportEntry(JSThread *thread, JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto sp = entryDataSp_;
    auto localExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    if (localExportNum == 0) {
        entryDataSp_ = sp;
        return;
    }
    SharedTypes sharedType = moduleRecord->GetSharedType();
    JSHandle<TaggedArray> localExportEntries = CreatEntries(thread, localExportNum, sharedType);

    uint32_t localIndex = -1;
    JSMutableHandle<JSTaggedValue> distinctLocalName(thread, JSTaggedValue::Undefined());
    for (size_t idx = 0; idx < localExportNum; idx++) {
        auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto exportNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto sd = pandaFile_->GetStringData(panda_file::File::EntityId(localNameOffset));
        JSHandle<JSTaggedValue> localName(thread, factory->GetRawStringFromStringTable(sd));

        sd = pandaFile_->GetStringData(panda_file::File::EntityId(exportNameOffset));
        JSHandle<JSTaggedValue> exportName(thread, factory->GetRawStringFromStringTable(sd));

        if (!JSTaggedValue::StrictEqual(thread, distinctLocalName, localName)) {
            distinctLocalName.Update(localName);
            localIndex++;
        }
        JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(exportName, localName, localIndex,
                                                                                   sharedType);
        localExportEntries->Set(thread, idx, localExportEntry);
    }
    entryDataSp_ = sp;
    moduleRecord->SetLocalExportEntries(thread, localExportEntries);
}

void ModuleDataAccessor::EnumerateIndirectExportEntry(JSThread *thread, JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConstants = thread->GlobalConstants();
    auto sp = entryDataSp_;

    auto indirectExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    if (indirectExportNum == 0) {
        entryDataSp_ = sp;
        return;
    }

    SharedTypes sharedType = moduleRecord->GetSharedType();
    JSHandle<TaggedArray> indirectExportEntries = CreatEntries(thread, indirectExportNum, sharedType);
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    for (size_t idx = 0; idx < indirectExportNum; idx++) {
        auto exportNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto importNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
        auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
        auto sd = pandaFile_->GetStringData(panda_file::File::EntityId(exportNameOffset));
        JSHandle<JSTaggedValue> exportName(thread, factory->GetRawStringFromStringTable(sd));

        sd = pandaFile_->GetStringData(panda_file::File::EntityId(importNameOffset));
        JSHandle<JSTaggedValue> importName(thread, factory->GetRawStringFromStringTable(sd));

        JSHandle<IndirectExportEntry> indirectExportEntry = factory->NewIndirectExportEntry(exportName,
            moduleRequestIdx, importName, sharedType);
        indirectExportEntries->Set(thread, idx, indirectExportEntry);
    }
    entryDataSp_ = sp;
    moduleRecord->SetIndirectExportEntries(thread, indirectExportEntries);
}

void ModuleDataAccessor::EnumerateStarExportEntry(JSThread *thread, JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConstants = thread->GlobalConstants();
    auto sp = entryDataSp_;

    uint32_t starExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
    if (starExportNum == 0) {
        entryDataSp_ = sp;
        return;
    }

    SharedTypes sharedType = moduleRecord->GetSharedType();
    JSHandle<TaggedArray> starExportEntries = CreatEntries(thread, starExportNum, sharedType);
    JSMutableHandle<JSTaggedValue> moduleRequest(thread, globalConstants->GetUndefined());
    for (uint32_t idx = 0; idx < starExportNum; idx++) {
        uint32_t moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));

        JSHandle<StarExportEntry> starExportEntry = factory->NewStarExportEntry(moduleRequestIdx, sharedType);
        starExportEntries->Set(thread, idx, starExportEntry.GetTaggedValue());
    }
    entryDataSp_ = sp;
    moduleRecord->SetStarExportEntries(thread, starExportEntries);
}
}  // namespace panda::ecmascript
