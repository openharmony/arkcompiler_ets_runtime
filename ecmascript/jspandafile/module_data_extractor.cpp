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

#include "libpandafile/class_data_accessor-inl.h"
#include "libpandafile/literal_data_accessor-inl.h"

namespace panda::ecmascript {
using StringData = panda_file::StringData;

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                         const CString &descriptor)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    Span<const uint32_t> classIndexes = pf->GetClasses();
    int moduleIdx = -1;
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        const char *desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (std::strcmp(JSPandaFile::MODULE_CLASS, desc) == 0) { // module class
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &field_accessor) -> void {
                panda_file::File::EntityId field_name_id = field_accessor.GetNameId();
                StringData sd = pf->GetStringData(field_name_id);
                if (std::strcmp(reinterpret_cast<const char *>(sd.data), descriptor.c_str())) {
                    moduleIdx = field_accessor.GetValue<int32_t>().value();
                    return;
                }
            });
        }
    }
    ASSERT(moduleIdx != -1);
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);
    panda_file::File::EntityId moduleId = lda.GetLiteralArrayId(static_cast<size_t>(moduleIdx));

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();
    ModuleDataExtractor::ExtractModuleDatas(thread, jsPandaFile, moduleId, moduleRecord);

    JSHandle<EcmaString> ecmaModuleFilename = factory->NewFromUtf8(descriptor);
    moduleRecord->SetEcmaModuleFilename(thread, ecmaModuleFilename);

    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::ECMAMODULE);

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

void ModuleDataExtractor::ExtractModuleDatas(JSThread *thread, const JSPandaFile *jsPandaFile,
                                             panda_file::File::EntityId moduleId,
                                             JSHandle<SourceTextModule> &moduleRecord)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ModuleDataAccessor mda(*pf, moduleId);
    const std::vector<uint32_t> &requestModules = mda.getRequestModules();
    JSHandle<TaggedArray> requestModuleArray = factory->NewTaggedArray(requestModules.size());
    for (size_t idx = 0; idx < requestModules.size(); idx++) {
        StringData sd = pf->GetStringData(panda_file::File::EntityId(requestModules[idx]));
        JSTaggedValue value(factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));
        requestModuleArray->Set(thread, idx, value);
    }
    if (requestModules.size()) {
        moduleRecord->SetRequestedModules(thread, requestModuleArray);
    }

    // note the order can't change
    mda.EnumerateImportEntry(thread, requestModuleArray, moduleRecord);
    mda.EnumerateLocalExportEntry(thread, moduleRecord);
    mda.EnumerateIndirectExportEntry(thread, requestModuleArray, moduleRecord);
    mda.EnumerateStarExportEntry(thread, requestModuleArray, moduleRecord);

    if (mda.GetNumExportEntry() <= SourceTextModule::DEFAULT_ARRAY_CAPACITY) {
        moduleRecord->SetModes(ModuleModes::ARRAYMODE);
    }
}

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseCjsModule(JSThread *thread, const CString &descriptor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();

    JSHandle<EcmaString> cjsModuleFilename = factory->NewFromUtf8(descriptor);
    moduleRecord->SetEcmaModuleFilename(thread, cjsModuleFilename);

    JSHandle<JSTaggedValue> defaultName = thread->GlobalConstants()->GetHandledDefaultString();
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(defaultName, defaultName);
    SourceTextModule::AddLocalExportEntry(thread, moduleRecord, localExportEntry, 0, 1); // 1 means len
    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::CJSMODULE);

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}
}  // namespace panda::ecmascript
