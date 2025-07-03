/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/module/module_snapshot.h"

#include "ecmascript/base/config.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/serializer/module_deserializer.h"
#include "ecmascript/serializer/module_serializer.h"
#include "securec.h"
#include "zlib.h"

namespace panda::ecmascript {
void ModuleSnapshot::SerializeDataAndPostSavingJob(const EcmaVM *vm, const CString &path, const CString &version)
{
    LOG_ECMA(INFO) << "ModuleSnapshot::SerializeDataAndPostSavingJob " << path;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::SerializeDataAndPostSavingJob", "");
    CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
    if (FileExist(filePath.c_str())) {
        LOG_ECMA(INFO) << "Module serialize file already exist";
        return;
    }
    JSThread *thread = vm->GetJSThread();
    std::unique_ptr<SerializeData> fileData = GetSerializeData(thread);
    if (fileData == nullptr) {
        return;
    }
    common::Taskpool::GetCurrentTaskpool()->PostTask(
        std::make_unique<ModuleSnapshotTask>(thread->GetThreadId(), thread, fileData, filePath, version));
}

bool ModuleSnapshot::DeserializeData(const EcmaVM *vm, const CString &path, const CString &version)
{
    LOG_ECMA(INFO) << "ModuleSnapshot::DeserializeData";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::DeserializeData", "");
    CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
    if (!FileExist(filePath.c_str())) {
        LOG_ECMA(INFO) << "ModuleSnapshot::DeserializeData Module serialize file doesn't exist: " << path;
        return false;
    }
    JSThread *thread = vm->GetJSThread();
    std::unique_ptr<SerializeData> fileData = std::make_unique<SerializeData>(thread);
    if (!ReadDataFromFile(thread, fileData, path, version)) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::DeserializeData failed: " << filePath;
        return false;
    }
    ModuleDeserializer deserializer(thread, fileData.release());
    JSHandle<TaggedArray> deserializedModules = JSHandle<TaggedArray>::Cast(deserializer.ReadValue());
    uint32_t length = deserializedModules->GetLength();
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue value = deserializedModules->Get(thread, i);
        JSHandle<SourceTextModule> moduleHdl(thread, SourceTextModule::Cast(value.GetTaggedObject()));
        CString moduleName = SourceTextModule::GetModuleName(moduleHdl.GetTaggedValue());
        if (SourceTextModule::IsSharedModule(moduleHdl)) {
            if (moduleHdl->GetStatus() == ModuleStatus::INSTANTIATED) {
                SharedModuleManager::GetInstance()->InsertInSModuleManager(thread, moduleName, moduleHdl);
            }
            continue;
        }
        ModuleManager *moduleManager = thread->GetModuleManager();
        moduleManager->AddResolveImportedModule(moduleName, value);
    }
    LOG_ECMA(INFO) << "ModuleSnapshot::DeserializeData success";
    return true;
}

JSHandle<TaggedArray> ModuleSnapshot::GetModuleSerializeArray(JSThread *thread)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    uint32_t normalModuleSize = moduleManager->GetResolvedModulesSize();
    uint32_t sharedModuleSize = SharedModuleManager::GetInstance()->GetResolvedSharedModulesSize();
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<TaggedArray> serializerArray = factory->NewTaggedArray(normalModuleSize + sharedModuleSize);
    moduleManager->AddNormalSerializeModule(thread, serializerArray, 0); // 0: start index
    SharedModuleManager::GetInstance()->AddSharedSerializeModule(thread, serializerArray, normalModuleSize);
    return serializerArray;
}

bool ModuleSnapshot::ModuleSnapshotTask::Run(uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshotTask", "");
    WriteDataToFile(thread_, serializeData_, path_, version_);
    return true;
}

void ModuleSnapshot::RemoveSnapshotFiles(const CString &path)
{
    DeleteFilesWithSuffix(path.c_str(), SNAPSHOT_FILE_SUFFIX.data());
}

std::unique_ptr<SerializeData> ModuleSnapshot::GetSerializeData(JSThread *thread)
{
    ModuleSerializer serializer(thread);
    JSHandle<TaggedArray> serializeArray = GetModuleSerializeArray(thread);
    const GlobalEnvConstants *globalConstants = thread->GlobalConstants();
    if (!serializer.WriteValue(thread, JSHandle<JSTaggedValue>(serializeArray),
                               globalConstants->GetHandledUndefined(),
                               globalConstants->GetHandledUndefined())) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::GetSerializeData serialize failed";
        return nullptr;
    }
    std::unique_ptr<SerializeData> fileData = serializer.Release();
    return fileData;
}

bool ModuleSnapshot::ReadDataFromFile(JSThread *thread, std::unique_ptr<SerializeData>& data, const CString& path,
    const CString &version)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::ReadDataFromFile", "");
    CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
    MemMap fileMapMem = FileMap(filePath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
    if (fileMapMem.GetOriginAddr() == nullptr) {
        RemoveSnapshotFiles(path);
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile File mmap failed";
        return false;
    }
    LOG_ECMA(INFO) << "ModuleSnapshot::ReadDataFromFile";
    MemMapScope memMapScope(fileMapMem);
    FileMemMapReader reader(fileMapMem, std::bind(RemoveSnapshotFiles, path), "ModuleSnapshot::ReadDataFromFile");
    uint32_t checksumSize = sizeof(uint32_t);
    uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t readCheckSum = 0;
    if (!reader.ReadFromOffset(&readCheckSum, checksumSize, contentSize, "checksum")) {
        return false;
    }
    uint32_t checksum = adler32(0, static_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    if (checksum != readCheckSum) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile checksum compare failed, checksum: " << checksum
            << ", readCheckSum" << readCheckSum;
        RemoveSnapshotFiles(path);
        return false;
    }
    // read app version
    uint32_t readAppVersionCode = 0;
    if (!reader.ReadSingleData(&readAppVersionCode, sizeof(readAppVersionCode), "AppVersionCode")) {
        return false;
    }
    uint32_t appVersionCode = thread->GetEcmaVM()->GetApplicationVersionCode();
    if (readAppVersionCode != appVersionCode) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile readAppVersionCode: " << readAppVersionCode <<
            ", appVersionCode: " << appVersionCode << " doesn't match";
        RemoveSnapshotFiles(path);
        return false;
    }
    // read version
    uint32_t readVersionStrLen = 0;
    if (!reader.ReadSingleData(&readVersionStrLen, sizeof(readVersionStrLen), "readVersionStrLen")) {
        return false;
    }
    CString readVersionStr;
    if (!reader.ReadString(readVersionStr, readVersionStrLen, "readVersionStr")) {
        return false;
    }
    if (version != readVersionStr) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile version compare failed, version: " << version
            << ", readVersion" << readVersionStr;
        RemoveSnapshotFiles(path);
        return false;
    }
    // read dataIndex
    if (!reader.ReadSingleData(&data->dataIndex_, sizeof(data->dataIndex_), "dataIndex")) {
        return false;
    }

    // 8-byte alignment
    reader.Step(GetAlignUpPadding(reader.GetReadPtr(), fileMapMem.GetOriginAddr(), sizeof(uint64_t)));

    // read uint64_t
    if (!reader.ReadSingleData(&data->sizeLimit_, sizeof(data->sizeLimit_), "sizeLimit")) {
        return false;
    }
    // 8-byte alignment
    reader.Step(GetAlignUpPadding(reader.GetReadPtr(), fileMapMem.GetOriginAddr(), sizeof(uint64_t)));
    // read group size
    size_t sizeGroup[GROUP_SIZE];
    if (!reader.ReadSingleData(sizeGroup, GROUP_SIZE * sizeof(size_t), "sizeGroup")) {
        return false;
    }
    data->bufferSize_ = sizeGroup[BUFFER_SIZE_INDEX];
    data->bufferCapacity_ = sizeGroup[BUFFER_CAPACITY_INDEX];
    data->regularSpaceSize_ = sizeGroup[REGULAR_SPACE_SIZE_INDEX];
    data->pinSpaceSize_ = sizeGroup[PIN_SPACE_SIZE_INDEX];
    data->oldSpaceSize_ = sizeGroup[OLD_SPACE_SIZE_INDEX];
    data->nonMovableSpaceSize_ = sizeGroup[NONMOVABLE_SPACE_SIZE_INDEX];
    data->machineCodeSpaceSize_ = sizeGroup[MACHINECODE_SPACE_SIZE_INDEX];
    data->sharedOldSpaceSize_ = sizeGroup[SHARED_OLD_SPACE_SIZE_INDEX];
    data->sharedNonMovableSpaceSize_ = sizeGroup[SHARED_NONMOVABLE_SPACE_SIZE_INDEX];

    // read and check imcompleteData
    const size_t incompleteData = sizeGroup[INCOMPLETE_DATA_INDEX];
    if (incompleteData != 0) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile has incompleteData: " << incompleteData;
        RemoveSnapshotFiles(path);
        return false;
    }
    data->incompleteData_ = (incompleteData != 0);

    if (g_isEnableCMCGC) {
        // read regularRemainSizeVectorSize
        size_t regularRemainSizeVectorSize;
        if (!reader.ReadSingleData(&regularRemainSizeVectorSize, sizeof(regularRemainSizeVectorSize),
            "regularRemainSizeVectorSize")) {
            return false;
        }
        // read regularRemainSizeVector
        size_t vecSize = regularRemainSizeVectorSize * sizeof(size_t);
        if (vecSize > 0) {
            data->regularRemainSizeVector_.resize(regularRemainSizeVectorSize);
            if (!reader.ReadSingleData(data->regularRemainSizeVector_.data(), vecSize,
                "regularRemainSizeVector")) {
                return false;
            }
        }
        // read pinRemainSizeVectorSize
        size_t pinRemainSizeVectorSize = 0;
        if (!reader.ReadSingleData(&pinRemainSizeVectorSize, sizeof(pinRemainSizeVectorSize),
            "pinRemainSizeVectorSize")) {
            return false;
        }
        // read pinRemainSizeVector
        vecSize = pinRemainSizeVectorSize * sizeof(size_t);
        if (vecSize > 0) {
            data->pinRemainSizeVector_.resize(pinRemainSizeVectorSize);
            if (!reader.ReadSingleData(data->pinRemainSizeVector_.data(), vecSize, "pinRemainSizeVector")) {
                return false;
            }
        }
    } else {
        // read vector size
        std::array<size_t, SERIALIZE_SPACE_NUM> vecSizes {};
        uint32_t vecSize = SERIALIZE_SPACE_NUM * sizeof(size_t);
        if (!reader.ReadSingleData(vecSizes.data(), vecSize, "vecSizes")) {
            return false;
        }
        // read each vector data
        for (int i = 0; i < SERIALIZE_SPACE_NUM; ++i) {
            auto& vec = data->regionRemainSizeVectors_[i];
            const size_t curVectorSize = vecSizes[i];
            const uint32_t curVectorDataSize = curVectorSize * sizeof(size_t);
            if (curVectorSize > 0) {
                vec.resize(curVectorSize);
                if (!reader.ReadSingleData(vec.data(), curVectorDataSize, "vec")) {
                    return false;
                }
            } else {
                vec.clear();
            }
        }
    }

    // read buffer data
    if (data->bufferSize_ > 0) {
        data->buffer_ = static_cast<uint8_t*>(malloc(data->bufferSize_));
        if (!data->buffer_) {
            RemoveSnapshotFiles(path);
            return false;
        }
        if (!reader.ReadSingleData(data->buffer_, data->bufferSize_, "buffer")) {
            return false;
        }
    } else {
        data->buffer_ = nullptr;
    }

    LOG_ECMA(INFO) << "ModuleSnapshot::ReadDataFromFile success";
    return true;
}

bool ModuleSnapshot::WriteDataToFile(
    JSThread *thread, const std::unique_ptr<SerializeData>& data, const CString& filePath, const CString &version)
{
    LOG_ECMA(INFO) << "ModuleSnapshot::WriteDataToFile";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::WriteDataToFile", "");
    // calculate file total size
    // versionCode
    uint32_t appVersionCode = thread->GetEcmaVM()->GetApplicationVersionCode();
    uint32_t totalSize = sizeof(appVersionCode);
    uint32_t versionStrLenSize = sizeof(uint32_t);
    uint32_t versionStrLen = version.size();
    totalSize += versionStrLenSize;
    totalSize += versionStrLen;

    totalSize += sizeof(data->dataIndex_);
    const size_t alignUp = AlignUp(totalSize, sizeof(uint64_t));
    totalSize = alignUp + sizeof(data->sizeLimit_);

    // alignment to size_t
    totalSize = AlignUp(totalSize, sizeof(size_t));
    // GROUP_SIZE
    totalSize += GROUP_SIZE * sizeof(size_t);

    if (g_isEnableCMCGC) {
        totalSize += CMC_GC_REGION_SIZE * sizeof(size_t);
        totalSize += data->regularRemainSizeVector_.size() * sizeof(size_t);
        totalSize += data->pinRemainSizeVector_.size() * sizeof(size_t);
    } else {
        // vector each element in vector's length
        totalSize += SERIALIZE_SPACE_NUM * sizeof(size_t);

        // vector data
        size_t totalVecBytes = 0;
        for (const auto& vec : data->regionRemainSizeVectors_) {
            totalVecBytes += vec.size() * sizeof(size_t);
        }
        totalSize += totalVecBytes;
    }
    // buffer data
    totalSize += data->bufferSize_;
    uint32_t checksumSize = sizeof(uint32_t);
    totalSize += checksumSize;
    MemMap fileMapMem =
        CreateFileMap(filePath.c_str(), totalSize, FILE_RDWR | FILE_CREAT | FILE_TRUNC, PAGE_PROT_READWRITE);
    if (fileMapMem.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile File mmap failed";
        return false;
    }
    MemMapScope memMapScope(fileMapMem);
    FileMemMapWriter writer(fileMapMem, "ModuleSnapshot::WriteDataToFile");

    // write app versionCode
    if (!writer.WriteSingleData(&appVersionCode, sizeof(appVersionCode), "appVersionCode")) {
        return false;
    }
    // write version
    if (!writer.WriteSingleData(&versionStrLen, versionStrLenSize, "versionStrLen")) {
        return false;
    }
    if (!writer.WriteSingleData(version.c_str(), versionStrLen, "versionStr")) {
        return false;
    }
    // write dataIndex
    if (!writer.WriteSingleData(&data->dataIndex_, sizeof(data->dataIndex_), "dataIndex")) {
        return false;
    }

    // padding
    if (!writer.WriteAlignUpPadding(GetAlignUpPadding(writer.GetWritePtr(),
        fileMapMem.GetOriginAddr(), sizeof(uint64_t)))) {
        return false;
    }
    // write uint64_t
    if (!writer.WriteSingleData(&data->sizeLimit_, sizeof(data->sizeLimit_), "sizeLimit")) {
        return false;
    }

    // alignment to size_t
    if (!writer.WriteAlignUpPadding(GetAlignUpPadding(writer.GetWritePtr(),
        fileMapMem.GetOriginAddr(), sizeof(uint64_t)))) {
        return false;
    }
    // constructor and write GROUP data(size_t)
    size_t sizeGroup[GROUP_SIZE] = {
        data->bufferSize_,
        data->bufferCapacity_,
        data->regularSpaceSize_,
        data->pinSpaceSize_,
        data->oldSpaceSize_,
        data->nonMovableSpaceSize_,
        data->machineCodeSpaceSize_,
        data->sharedOldSpaceSize_,
        data->sharedNonMovableSpaceSize_,
        static_cast<size_t>(data->incompleteData_)
    };
    if (!writer.WriteSingleData(sizeGroup, GROUP_SIZE * sizeof(size_t), "sizeGroup")) {
        return false;
    }

    if (g_isEnableCMCGC) {
        totalSize += data->regularRemainSizeVector_.size() * sizeof(size_t);
        totalSize += data->pinRemainSizeVector_.size() * sizeof(size_t);
        size_t regularRemainSize = data->regularRemainSizeVector_.size();
        // regularRemainSizeVector size
        if (!writer.WriteSingleData(&regularRemainSize, sizeof(regularRemainSize), "regularRemainSize")) {
            return false;
        }
        // regularRemainSizeVector
        size_t regularRemainLen = regularRemainSize * sizeof(size_t);
        if (regularRemainLen > 0) {
            if (!writer.WriteSingleData(data->regularRemainSizeVector_.data(), regularRemainLen,
                "regularRemainSizeVector")) {
                return false;
            }
        }
        // pinRemainSizeVector size
        size_t pinRemainSize = data->pinRemainSizeVector_.size();
        if (!writer.WriteSingleData(&pinRemainSize, sizeof(pinRemainSize), "pinRemainSize")) {
            return false;
        }
        // pinRemainSizeVector
        size_t pinRemainLen = pinRemainSize * sizeof(size_t);
        if (pinRemainLen > 0) {
            if (!writer.WriteSingleData(data->pinRemainSizeVector_.data(), pinRemainLen, "pinRemainSizeVector")) {
                return false;
            }
        }
    } else {
        // write vector's size
        std::array<size_t, SERIALIZE_SPACE_NUM> vecSizes;
        for (int i = 0; i < SERIALIZE_SPACE_NUM; ++i) {
            vecSizes[i] = data->regionRemainSizeVectors_[i].size();
        }
        uint32_t vecSize = SERIALIZE_SPACE_NUM * sizeof(size_t);
        if (!writer.WriteSingleData(vecSizes.data(), vecSize, "vecSizes")) {
            return false;
        }

        // write vector's data
        for (const auto& vec : data->regionRemainSizeVectors_) {
            if (!vec.empty()) {
                uint32_t curVectorDataSize = vec.size() * sizeof(size_t);
                if (!writer.WriteSingleData(vec.data(), curVectorDataSize, "vec")) {
                    return false;
                }
            }
        }
    }
    // write buffer data
    if (data->bufferSize_ > 0) {
        if (!data->buffer_) {
            return false;
        }
        if (!writer.WriteSingleData(data->buffer_, data->bufferSize_, "buffer")) {
            return false;
        }
    }
    uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t checksum = adler32(0, static_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    if (!writer.WriteSingleData(&checksum, checksumSize, "checksum")) {
        return false;
    }
    FileSync(fileMapMem, FILE_MS_SYNC);
    LOG_ECMA(INFO) << "ModuleSnapshot::WriteDataToFile success";
    return true;
}
} // namespace panda::ecmascript
