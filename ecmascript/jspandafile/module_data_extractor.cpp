/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/jspandafile/accessor/module_data_accessor.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/module/js_module_manager.h"

#include "libpandafile/literal_data_accessor-inl.h"

namespace panda::ecmascript {
using StringData = panda_file::StringData;

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                         const CString &descriptor, const CString &moduleFilename)
{
    int moduleIdx = jsPandaFile->GetModuleRecordIdx(descriptor);
    ASSERT(moduleIdx != -1);
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);
    panda_file::File::EntityId moduleId;
    if (jsPandaFile->IsNewVersion()) {  // new pandafile version use new literal offset mechanism
        moduleId = panda_file::File::EntityId(static_cast<uint32_t>(moduleIdx));
    } else {
        moduleId = lda.GetLiteralArrayId(static_cast<size_t>(moduleIdx));
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();
    ModuleDataExtractor::ExtractModuleDatas(thread, jsPandaFile, moduleId, moduleRecord);

    JSHandle<EcmaString> ecmaModuleFilename = factory->NewFromUtf8(moduleFilename);
    moduleRecord->SetEcmaModuleFilename(thread, ecmaModuleFilename);

    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::ECMAMODULE);
    moduleRecord->SetIsNewBcVersion(jsPandaFile->IsNewVersion());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

void ModuleDataExtractor::ExtractModuleDatas(JSThread *thread, const JSPandaFile *jsPandaFile,
                                             panda_file::File::EntityId moduleId,
                                             JSHandle<SourceTextModule> &moduleRecord)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ModuleDataAccessor mda(*pf, moduleId);
    const std::vector<uint32_t> &requestModules = mda.getRequestModules();
    size_t len = requestModules.size();
    JSHandle<TaggedArray> requestModuleArray = factory->NewTaggedArray(len);
    for (size_t idx = 0; idx < len; idx++) {
        StringData sd = pf->GetStringData(panda_file::File::EntityId(requestModules[idx]));
        JSTaggedValue value(factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));
        requestModuleArray->Set(thread, idx, value);
    }
    if (len > 0) {
        moduleRecord->SetRequestedModules(thread, requestModuleArray);
    }

    // note the order can't change
    mda.EnumerateImportEntry(thread, requestModuleArray, moduleRecord);
    mda.EnumerateLocalExportEntry(thread, moduleRecord);
    mda.EnumerateIndirectExportEntry(thread, requestModuleArray, moduleRecord);
    mda.EnumerateStarExportEntry(thread, requestModuleArray, moduleRecord);
}

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseCjsModule(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    const CString &descriptor = jsPandaFile->GetJSPandaFileDesc();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();

    JSHandle<EcmaString> cjsModuleFilename = factory->NewFromUtf8(descriptor);
    moduleRecord->SetEcmaModuleFilename(thread, cjsModuleFilename);

    JSHandle<JSTaggedValue> defaultName = thread->GlobalConstants()->GetHandledDefaultString();
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(defaultName, defaultName);
    SourceTextModule::AddLocalExportEntry(thread, moduleRecord, localExportEntry, 0, 1); // 1 means len
    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::CJSMODULE);
    moduleRecord->SetIsNewBcVersion(jsPandaFile->IsNewVersion());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseJsonModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                             const CString &moduleFilename, const CString &recordName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();

    JSHandle<JSTaggedValue> defaultName = thread->GlobalConstants()->GetHandledDefaultString();
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(defaultName, defaultName);
    SourceTextModule::AddLocalExportEntry(thread, moduleRecord, localExportEntry, 0, 1); // 1 means len
    JSTaggedValue jsonData = ModuleManager::JsonParse(thread, jsPandaFile, recordName);
    moduleRecord->StoreModuleValue(thread, 0, JSHandle<JSTaggedValue>(thread, jsonData)); // index = 0

    JSHandle<EcmaString> ecmaModuleFilename = factory->NewFromUtf8(moduleFilename);
    moduleRecord->SetEcmaModuleFilename(thread, ecmaModuleFilename);

    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::JSONMODULE);
    moduleRecord->SetIsNewBcVersion(jsPandaFile->IsNewVersion());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}
}  // namespace panda::ecmascript
