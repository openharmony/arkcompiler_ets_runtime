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

#include "ecmascript/jspandafile/accessor/module_data_accessor.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/module_data_extractor.h"

#include "libpandafile/literal_data_accessor-inl.h"

namespace panda::ecmascript {
using StringData = panda_file::StringData;
using BuiltinsJson = builtins::BuiltinsJson;
using JSRecordInfo = ecmascript::JSPandaFile::JSRecordInfo;

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                         const CString &descriptor, const CString &moduleFilename)
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
    ModuleDataExtractor::ExtractModuleDatas(thread, jsPandaFile, moduleId, moduleRecord);

    JSHandle<EcmaString> ecmaModuleFilename = factory->NewFromUtf8(moduleFilename);
    moduleRecord->SetEcmaModuleFilename(thread, ecmaModuleFilename);

    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::ECMA_MODULE);
    moduleRecord->SetIsNewBcVersion(jsPandaFile->IsNewVersion());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

void ModuleDataExtractor::ExtractModuleDatas(JSThread *thread, const JSPandaFile *jsPandaFile,
                                             panda_file::File::EntityId moduleId,
                                             JSHandle<SourceTextModule> &moduleRecord)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ModuleDataAccessor mda(jsPandaFile, moduleId);
    const std::vector<uint32_t> &requestModules = mda.getRequestModules();
    size_t len = requestModules.size();
    JSHandle<TaggedArray> requestModuleArray = factory->NewTaggedArray(len);
    for (size_t idx = 0; idx < len; idx++) {
        StringData sd = jsPandaFile->GetStringData(panda_file::File::EntityId(requestModules[idx]));
        JSTaggedValue value(factory->GetRawStringFromStringTable(sd));
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
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(defaultName,
        defaultName, LocalExportEntry::LOCAL_DEFAULT_INDEX);
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
        defaultName, LocalExportEntry::LOCAL_DEFAULT_INDEX);
    SourceTextModule::AddLocalExportEntry(thread, moduleRecord, localExportEntry, 0, 1); // 1 means len
    JSTaggedValue jsonData = JsonParse(thread, jsPandaFile, recordName);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    moduleRecord->StoreModuleValue(thread, 0, JSHandle<JSTaggedValue>(thread, jsonData)); // index = 0

    JSHandle<EcmaString> ecmaModuleFilename = factory->NewFromUtf8(moduleFilename);
    moduleRecord->SetEcmaModuleFilename(thread, ecmaModuleFilename);

    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    moduleRecord->SetTypes(ModuleTypes::JSON_MODULE);
    moduleRecord->SetIsNewBcVersion(jsPandaFile->IsNewVersion());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseNativeModule(JSThread *thread, const CString &moduleRequestName,
                                                               ModuleTypes moduleType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();

    // set moduleRecordName as non-undefined to distinguish between merge and non-merge mode
    JSHandle<EcmaString> falsyRecordName = factory->NewFromUtf8(moduleRequestName);
    moduleRecord->SetEcmaModuleRecordName(thread, falsyRecordName);
    JSHandle<JSTaggedValue> defaultName = thread->GlobalConstants()->GetHandledDefaultString();
    JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry(defaultName,
        defaultName, LocalExportEntry::LOCAL_DEFAULT_INDEX);
    SourceTextModule::AddLocalExportEntry(thread, moduleRecord, localExportEntry, 0, 1);
    moduleRecord->SetTypes(moduleType);
    moduleRecord->SetIsNewBcVersion(true);
    moduleRecord->SetStatus(ModuleStatus::INSTANTIATED);
    moduleRecord->StoreModuleValue(thread, 0, thread->GlobalConstants()->GetHandledUndefined());

    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

JSTaggedValue ModuleDataExtractor::JsonParse(JSThread *thread, const JSPandaFile *jsPandaFile, CString entryPoint)
{
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(
            thread, undefined, undefined, undefined, 1); // 1 : argument numbers
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSRecordInfo recordInfo;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entryPoint, recordInfo);
    if (!hasRecord) {
        CString msg = "cannot find record '" + entryPoint + "', please check the request path.";
        LOG_FULL(ERROR) << msg;
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), JSTaggedValue::Exception());
    }
    CString value = jsPandaFile->GetJsonStringId(recordInfo);
    JSHandle<JSTaggedValue> arg0(thread->GetEcmaVM()->GetFactory()->NewFromASCII(value));
    info->SetCallArg(arg0.GetTaggedValue());
    return BuiltinsJson::Parse(info);
}
}  // namespace panda::ecmascript
