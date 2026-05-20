/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "constpool_snapshot.h"

#include "ecmascript/base/config.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/signal_manager.h"
#include "ecmascript/serializer/file_deserializer.h"
#include "ecmascript/serializer/file_serializer.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#include "ecmascript/jspandafile/program_object.h"

#include <securec.h>
#include <zlib.h>
#include <cstdint>

using IndexHeader = panda::panda_file::File::IndexHeader;

namespace panda::ecmascript {
void ConstPoolSnapshot::SerializeDataAndPostSavingJob(const EcmaVM* vm, JSPandaFile* pandafile, const CString& path,
                                                      const CString& version)
{
    LOG_ECMA(DEBUG) << "ConstPoolSnapshot::SerializeDataAndPostSavingJob " << path;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConstPoolSnapshot::SerializeDataAndPostSavingJob",
                      "");
    CString filePath = base::ConcatToCString(path, GetSnapshotFileName(pandafile));
    if (FileExist(filePath.c_str())) {
        LOG_ECMA(INFO) << "ConstPool serialize file already exist";
        return;
    }
    ModulesSnapshotHelper::MarkConstPoolSnapshotLoaded();
    JSThread* thread = vm->GetJSThread();
    std::unique_ptr<SerializeData> fileData = GetSerializeData(thread, pandafile);
    if (fileData == nullptr) {
        return;
    }
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(thread->GetEcmaVM()->GetApplicationVersionCode(),
                                                                  version, pandafile->GetJSPandaFileDesc());
    common::Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SnapshotSaveTask>(
        thread->GetThreadId(), std::move(fileData), filePath, std::move(header), "ConstPoolSnapshot"));
}

bool ConstPoolSnapshot::DeserializeData(EcmaVM* vm, JSPandaFile* pandafile, const CString& path, const CString& version)
{
    if (pandafile == nullptr) {
        return false;
    }
    if (ModulesSnapshotHelper::IsConstPoolSnapshotDisabled(path)) {
        LOG_ECMA(DEBUG) << "ConstPoolSnapshot::DeserializeData ConstPool Snapshot is not enabled";
        ModulesSnapshotHelper::HandleCorruptedFile(path, CString(ConstPoolSnapshot::CONSTPOOL_SNAPSHOT_FILE_SUFFIX));
        return false;
    }
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConstPoolSnapshot::DeserializeData", "");
    LOG_ECMA(DEBUG) << "ConstPoolSnapshot::DeserializeData";
    CString filePath = base::ConcatToCString(path, GetSnapshotFileName(pandafile));
    if (!FileExist(filePath.c_str())) {
        LOG_ECMA(INFO) << "ConstPoolSnapshot::DeserializeData Constpool serialize file doesn't exist: " << filePath;
        return false;
    }
    ModulesSnapshotHelper::MarkConstPoolSnapshotLoaded();
    JSThread* thread = vm->GetJSThread();
    std::unique_ptr<SerializeData> fileData = std::make_unique<SerializeData>(thread);
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(vm->GetApplicationVersionCode(), version,
                                                                  pandafile->GetJSPandaFileDesc());
    if (!ModulesSnapshotHelper::ReadDataFromFile(fileData, filePath, header, "ConstPoolSnapshot")) {
        LOG_ECMA(ERROR) << "ConstPoolSnapshot::DeserializeData failed: " << filePath;
        ModulesSnapshotHelper::RemoveFile(filePath);
        return false;
    }
    FileDeserializer deserializer(thread, fileData.get());
    EcmaHandleScope scope(thread);
    JSHandle<TaggedArray> deserializedConstpools = JSHandle<TaggedArray>::Cast(deserializer.ReadValue());
    if (!deserializedConstpools.GetTaggedValue().IsTaggedArray()) {
        LOG_ECMA(ERROR) << "ConstPoolSnapshot::DeserializeData failed: deserializedConstpools is not tagged array";
        ModulesSnapshotHelper::RemoveFile(filePath);
        return false;
    }
    uint32_t length = deserializedConstpools->GetLength();
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue constpool = deserializedConstpools->Get(thread, i);
        if (!constpool.IsConstantPool()) {
            LOG_ECMA(WARN) << "ConstPoolSnapshot::DeserializeData: invalid constpool";
            continue;
        }
        JSHandle<ConstantPool> constpoolHdl(thread, constpool);
        JSTaggedValue taggedConstpoolId = constpoolHdl->GetSharedConstpoolId();
        constpoolHdl->InitializeWithSpecialValue(thread);
        int sharedId = taggedConstpoolId.GetInt();
        auto idxHeaders = pandafile->GetPandaFile()->GetIndexHeaders();
        if (static_cast<uint32_t>(sharedId) >= idxHeaders.size()) {
            LOG_ECMA(ERROR) << "ConstPoolSnapshot::DeserializeData failed: sharedId out of range";
            ModulesSnapshotHelper::RemoveFile(filePath);
            return false;
        }
        const IndexHeader* idxHeader = &idxHeaders[sharedId];
        constpoolHdl->SetJSPandaFile(pandafile);
        constpoolHdl->SetIndexHeader(idxHeader);
        constpoolHdl->SetSharedConstpoolId(taggedConstpoolId);
        vm->AddOrUpdateConstpool(pandafile, constpoolHdl, static_cast<int32_t>(sharedId));
        vm->CreateUnsharedConstpool(constpool);
    }
    LOG_ECMA(INFO) << "ConstPoolSnapshot::DeserializeData success";
    return true;
}

JSHandle<JSTaggedValue> ConstPoolSnapshot::GetSerializeArray(JSThread* thread, JSPandaFile* pandafile)
{
    JSHandle<JSTaggedValue> serializedVal(thread, JSTaggedValue::Hole());
    uint32_t idx = 0;
    thread->GetEcmaVM()->ForEachSharedConstpool(
        pandafile, [thread, &serializedVal, &idx]([[maybe_unused]] int32_t index, JSTaggedValue value, size_t total) {
            if (serializedVal->IsHole()) {
                ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
                serializedVal = JSHandle<JSTaggedValue>(factory->NewTaggedArray(total));
            }
            TaggedArray::Cast(serializedVal.GetTaggedValue())->Set(thread, idx++, value);
        });
    return serializedVal;
}

std::unique_ptr<SerializeData> ConstPoolSnapshot::GetSerializeData(JSThread* thread, JSPandaFile* pandafile)
{
    FileSerializer serializer(thread);
    EcmaHandleScope scope(thread);
    JSHandle<JSTaggedValue> serializeArray = GetSerializeArray(thread, pandafile);
    if (!serializeArray->IsTaggedArray()) {
        LOG_ECMA(ERROR) << "ConstPoolSnapshot::GetSerializeData serializeArray is not tagged array";
        return nullptr;
    }
    const GlobalEnvConstants* globalConstants = thread->GlobalConstants();
    if (!serializer.WriteValue(thread, JSHandle<JSTaggedValue>(serializeArray), globalConstants->GetHandledUndefined(),
                               globalConstants->GetHandledUndefined())) {
        LOG_ECMA(ERROR) << "ConstPoolSnapshot::GetSerializeData serialize failed";
        return nullptr;
    }
    std::unique_ptr<SerializeData> fileData = serializer.Release();
    return fileData;
}

CString ConstPoolSnapshot::GetSnapshotFileName(const JSPandaFile* pandafile)
{
    return base::ConcatToCString(ModulePathHelper::GetModuleNameWithBaseFile(pandafile->GetJSPandaFileDesc()),
                                 CONSTPOOL_SNAPSHOT_FILE_SUFFIX);
}

} // namespace panda::ecmascript
