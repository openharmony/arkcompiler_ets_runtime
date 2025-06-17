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
void ModuleSnapshot::SerializeDataAndPostSavingJob(const EcmaVM *vm, const CString &path)
{
    LOG_ECMA(INFO) << "ModuleSnapshot::SerializeDataAndPostSavingJob " << path;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::SerializeDataAndPostSavingJob", "");
    CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
    if (FileExist(filePath.c_str())) {
        LOG_ECMA(INFO) << "Module serialize file already exist";
        return;
    }
    JSThread *thread = vm->GetJSThread();
    ModuleSerializer serializer(thread);
    JSHandle<TaggedArray> serializeArray = GetModuleSerializeArray(thread);
    const GlobalEnvConstants *globalConstants = thread->GlobalConstants();
    if (!serializer.WriteValue(thread, JSHandle<JSTaggedValue>(serializeArray),
                               globalConstants->GetHandledUndefined(),
                               globalConstants->GetHandledUndefined())) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::SerializeDataAndPostSavingJob serialize failed";
        return;
    }
    std::unique_ptr<SerializeData> fileData = serializer.Release();
    common::Taskpool::GetCurrentTaskpool()->PostTask(
        std::make_unique<ModuleSnapshotTask>(thread->GetThreadId(), thread, fileData, filePath));
}

void ModuleSnapshot::DeserializeData(const EcmaVM *vm, const CString &path)
{
    LOG_ECMA(INFO) << "ModuleSnapshot::DeserializeData";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::DeserializeData", "");
    CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
    if (!FileExist(filePath.c_str())) {
        LOG_ECMA(INFO) << "ModuleSnapshot::DeserializeData Module serialize file doesn't exist: " << path;
        return;
    }
    JSThread *thread = vm->GetJSThread();
    std::unique_ptr<SerializeData> fileData = std::make_unique<SerializeData>(thread);
    if (!ReadDataFromFile(thread, fileData, path)) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::DeserializeData failed: " << filePath;
        return;
    }
    ModuleDeserializer deserializer(thread, fileData.release());
    JSHandle<TaggedArray> deserializedModules = JSHandle<TaggedArray>::Cast(deserializer.ReadValue());
    uint32_t length = deserializedModules->GetLength();
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue value = deserializedModules->Get(i);
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
    WriteDataToFile(thread_, serializeData_, path_);
    return true;
}

void ModuleSnapshot::RemoveSnapshotFiles(const CString &path)
{
    DeleteFilesWithSuffix(path.c_str(), SNAPSHOT_FILE_SUFFIX.data());
}

bool ModuleSnapshot::ReadDataFromFile(JSThread *thread, std::unique_ptr<SerializeData>& data, const CString& path)
{
    LOG_ECMA(INFO) << "ModuleSnapshot::ReadDataFromFile";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::ReadDataFromFile", "");
    CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
    MemMap fileMapMem = FileMap(filePath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
    if (fileMapMem.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile File mmap failed";
        return false;
    }
    MemMapScope memMapScope(fileMapMem);
    uint8_t* readPtr = static_cast<uint8_t*>(fileMapMem.GetOriginAddr());
    size_t checksumSize = sizeof(uint32_t);
    uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t checksum = adler32(0, reinterpret_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    uint32_t readCheckSum = *reinterpret_cast<const uint32_t*>(readPtr + contentSize);
    if (checksum != readCheckSum) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile checksum compare failed, checksum: " << checksum
            << ", readCheckSum" << readCheckSum;
        RemoveSnapshotFiles(path);
        return false;
    }
    size_t remaining = fileMapMem.GetSize();
    // read app version
    uint32_t readAppVersionCode;
    if (remaining < sizeof(readAppVersionCode)) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile read AppVersionCode failed";
        RemoveSnapshotFiles(path);
        return false;
    }
    if (memcpy_s(&readAppVersionCode, sizeof(readAppVersionCode), readPtr, sizeof(readAppVersionCode)) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile memcpy_s read AppVersionCode failed";
        return false;
    }
    readPtr += sizeof(readAppVersionCode);
    remaining -= sizeof(readAppVersionCode);
    uint32_t appVersionCode = thread->GetEcmaVM()->GetApplicationVersionCode();
    if (readAppVersionCode != appVersionCode) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile readAppVersionCode: " << readAppVersionCode <<
            ", appVersionCode: " << appVersionCode << " doesn't match";
        RemoveSnapshotFiles(path);
        return false;
    }

    // read module version
    uint32_t snapshotVersionCode;
    if (remaining < sizeof(snapshotVersionCode)) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile read snapshotVersionCode failed";
        RemoveSnapshotFiles(path);
        return false;
    }
    if (memcpy_s(&snapshotVersionCode, sizeof(snapshotVersionCode), readPtr, sizeof(snapshotVersionCode)) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile memcpy_s read snapshotVersionCode failed";
        return false;
    }
    if (snapshotVersionCode != GetVersionCode()) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile snapshotVersionCode: " << snapshotVersionCode <<
            ", MODULE_VERSION_CODE: " << GetVersionCode() << " doesn't match.";
        RemoveSnapshotFiles(path);
        return false;
    }
    readPtr += sizeof(snapshotVersionCode);
    remaining -= sizeof(snapshotVersionCode);

    // read dataIndex
    if (remaining < sizeof(data->dataIndex_)) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile read dataIndex failed";
        RemoveSnapshotFiles(path);
        return false;
    }
    if (memcpy_s(&data->dataIndex_, sizeof(data->dataIndex_), readPtr, sizeof(data->dataIndex_)) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile memcpy_s dataIndex failed";
        return false;
    }
    readPtr += sizeof(data->dataIndex_);
    remaining -= sizeof(data->dataIndex_);

    // 8-byte alignment
    const size_t alignPadding = AlignUp(readPtr - static_cast<uint8_t*>(fileMapMem.GetOriginAddr()), sizeof(uint64_t))
                                - (readPtr - static_cast<uint8_t*>(fileMapMem.GetOriginAddr()));
    readPtr += alignPadding;
    remaining -= alignPadding;

    // read uint64_t
    if (remaining < sizeof(data->sizeLimit_)) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile read sizeLimit failed";
        RemoveSnapshotFiles(path);
        return false;
    }
    if (memcpy_s(&data->sizeLimit_, sizeof(data->sizeLimit_), readPtr, sizeof(data->sizeLimit_)) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile memcpy_s sizeLimit failed";
        return false;
    }
    readPtr += sizeof(data->sizeLimit_);
    remaining -= sizeof(data->sizeLimit_);

    // 8-byte alignment
    readPtr = static_cast<uint8_t*>(fileMapMem.GetOriginAddr()) +
               AlignUp(readPtr - static_cast<uint8_t*>(fileMapMem.GetOriginAddr()), sizeof(size_t));
    remaining = fileMapMem.GetSize() - (readPtr - static_cast<uint8_t*>(fileMapMem.GetOriginAddr()));
    if (!g_isEnableCMCGC) {
        // read group size
        constexpr size_t SIZE_T_GROUP_SIZE = GROUP_SIZE * sizeof(size_t);
        if (remaining < SIZE_T_GROUP_SIZE) {
            LOG_ECMA(ERROR) << "read SIZE_T_GROUP_SIZE failed";
            RemoveSnapshotFiles(path);
            return false;
        }

        const size_t* sizeGroup = reinterpret_cast<size_t*>(readPtr);
        data->bufferSize_ = sizeGroup[BUFFER_SIZE_INDEX];
        data->bufferCapacity_ = sizeGroup[BUFFER_CAPACITY_INDEX];
        data->oldSpaceSize_ = sizeGroup[OLD_SPACE_SIZE_INDEX];
        data->nonMovableSpaceSize_ = sizeGroup[NONMOVABLE_SPACE_SIZE_INDEX];
        data->machineCodeSpaceSize_ = sizeGroup[MACHINECODE_SPACE_SIZE_INDEX];
        data->sharedOldSpaceSize_ = sizeGroup[SHARED_OLD_SPACE_SIZE_INDEX];
        data->sharedNonMovableSpaceSize_ = sizeGroup[SHARED_NONMOVABLE_SPACE_SIZE_INDEX];

        // read and check imcompleteData
        const size_t incompleteData = sizeGroup[INCOMPLETE_DATA_INDEX];
        if (incompleteData > 1) {
            LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile read incompleteData failed";
            RemoveSnapshotFiles(path);
            return false;
        }
        data->incompleteData_ = (incompleteData != 0);

        readPtr += SIZE_T_GROUP_SIZE;
        remaining -= SIZE_T_GROUP_SIZE;

        // read vector size
        std::array<size_t, SERIALIZE_SPACE_NUM> vecSizes;
        if (remaining < SERIALIZE_SPACE_NUM * sizeof(size_t)) {
            LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile read vecSizes failed";
            RemoveSnapshotFiles(path);
            return false;
        }
        if (memcpy_s(vecSizes.data(), SERIALIZE_SPACE_NUM * sizeof(size_t),
                     readPtr, SERIALIZE_SPACE_NUM * sizeof(size_t)) != EOK) {
            LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile memcpy_s vecSizes failed";
            return false;
        }
        readPtr += SERIALIZE_SPACE_NUM * sizeof(size_t);
        remaining -= SERIALIZE_SPACE_NUM * sizeof(size_t);

        // check vector data size
        const size_t totalVecBytes = std::accumulate(
            vecSizes.begin(), vecSizes.end(), 0UL,
            [](size_t sum, size_t sz) { return sum + sz * sizeof(size_t); }
        );
        if (remaining < totalVecBytes) {
            LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile read totalVecBytes failed";
            RemoveSnapshotFiles(path);
            return false;
        }

        // check each vector data
        for (int i = 0; i < SERIALIZE_SPACE_NUM; ++i) {
            auto& vec = data->regionRemainSizeVectors_[i];
            const size_t vec_size = vecSizes[i];

            if (vec_size > 0) {
                vec.resize(vec_size);
                if (memcpy_s(vec.data(), vec_size * sizeof(size_t), readPtr, vec_size * sizeof(size_t)) != EOK) {
                    LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile memcpy_s vecSizes failed";
                    return false;
                }
                readPtr += vec_size * sizeof(size_t);
            } else {
                vec.clear();
            }
        }
        remaining -= totalVecBytes;
    }

    // read buffer data
    if (data->bufferSize_ > 0) {
        if (remaining < data->bufferSize_) {
            RemoveSnapshotFiles(path);
            return false;
        }
        data->buffer_ = static_cast<uint8_t*>(malloc(data->bufferSize_));
        if (!data->buffer_) {
            RemoveSnapshotFiles(path);
            return false;
        }
        if (memcpy_s(data->buffer_, data->bufferSize_, readPtr, data->bufferSize_) != EOK) {
            LOG_ECMA(ERROR) << "ModuleSnapshot::ReadDataFromFile memcpy_s vecSizes failed";
            return false;
        }
    } else {
        data->buffer_ = nullptr;
    }

    LOG_ECMA(INFO) << "ModuleSnapshot::ReadDataFromFile success";
    return true;
}

bool ModuleSnapshot::WriteDataToFile(
    JSThread *thread, const std::unique_ptr<SerializeData>& data, const CString& filePath)
{
    LOG_ECMA(INFO) << "ModuleSnapshot::WriteDataToFile";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshot::WriteDataToFile", "");
    // calculate file total size
    // versionCode
    uint32_t appVersionCode = thread->GetEcmaVM()->GetApplicationVersionCode();
    size_t totalSize = sizeof(appVersionCode);
    uint32_t curModuleVersionCode = GetVersionCode();
    totalSize += sizeof(curModuleVersionCode);

    totalSize += sizeof(data->dataIndex_);
    const size_t alignUp = AlignUp(totalSize, sizeof(uint64_t));
    totalSize = alignUp + sizeof(data->sizeLimit_);

    // alignment to size_t
    totalSize = AlignUp(totalSize, sizeof(size_t));
    if (!g_isEnableCMCGC) {
        // GROUP_SIZE
        totalSize += GROUP_SIZE * sizeof(size_t);

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
    uint8_t* writePtr = static_cast<uint8_t*>(fileMapMem.GetOriginAddr());

    // write app versionCode
    if (memcpy_s(writePtr, sizeof(appVersionCode), &appVersionCode, sizeof(appVersionCode)) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memcpy_s write appVersionCode failed";
        return false;
    }
    writePtr += sizeof(appVersionCode);

    // write module versionCode
    if (memcpy_s(writePtr, sizeof(curModuleVersionCode),
        &curModuleVersionCode, sizeof(curModuleVersionCode)) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memcpy_s write module versionCode failed";
        return false;
    }
    writePtr += sizeof(curModuleVersionCode);

    // write dataIndex
    if (memcpy_s(writePtr, sizeof(data->dataIndex_), &data->dataIndex_, sizeof(data->dataIndex_)) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memcpy_s write dataIndex failed";
        return false;
    }
    writePtr += sizeof(data->dataIndex_);

    // padding
    const size_t pad1 = AlignUp(writePtr - static_cast<uint8_t*>(fileMapMem.GetOriginAddr()), sizeof(uint64_t))
                        - (writePtr - static_cast<uint8_t*>(fileMapMem.GetOriginAddr()));
    if (pad1 > 0) {
        if (memset_s(writePtr, pad1, 0, pad1) != EOK) { // 0: reset
            LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memset_s write pad1 failed";
            return false;
        }
        writePtr += pad1;
    }
    // write uint64_t
    if (memcpy_s(writePtr, sizeof(data->sizeLimit_), &data->sizeLimit_, sizeof(data->sizeLimit_)) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memcpy_s write sizeLimit failed";
        return false;
    }
    writePtr += sizeof(data->sizeLimit_);

    // alignment to size_t
    const size_t pad2 = AlignUp(writePtr - static_cast<uint8_t*>(fileMapMem.GetOriginAddr()), sizeof(uint64_t))
                        - (writePtr - static_cast<uint8_t*>(fileMapMem.GetOriginAddr()));
    if (pad2 > 0) {
        if (memset_s(writePtr, pad2, 0, pad2) != EOK) { // 0: reset
            LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memset_s write pad2 failed";
            return false;
        }
        writePtr += pad2;
    }
    if (!g_isEnableCMCGC) {
        // constructor and write GROUP data(size_t)
        size_t sizeGroup[GROUP_SIZE] = {
            data->bufferSize_,
            data->bufferCapacity_,
            data->oldSpaceSize_,
            data->nonMovableSpaceSize_,
            data->machineCodeSpaceSize_,
            data->sharedOldSpaceSize_,
            data->sharedNonMovableSpaceSize_,
            static_cast<size_t>(data->incompleteData_)
        };
        if (memcpy_s(writePtr, sizeof(sizeGroup), sizeGroup, sizeof(sizeGroup)) != EOK) {
            LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memcpy_s write sizeGroup failed";
            return false;
        }
        writePtr += sizeof(sizeGroup);

        // write 7 vector's size
        std::array<size_t, SERIALIZE_SPACE_NUM> vecSizes;
        for (int i = 0; i < SERIALIZE_SPACE_NUM; ++i) {
            vecSizes[i] = data->regionRemainSizeVectors_[i].size();
        }
        uint32_t vecSizeLength = vecSizes.size() * sizeof(size_t);
        if (memcpy_s(writePtr, vecSizeLength, vecSizes.data(), vecSizeLength) != EOK) {
            LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memcpy_s write vecSizes failed";
            return false;
        }
        writePtr += vecSizes.size() * sizeof(size_t);

        // write 7 vector's data
        for (const auto& vec : data->regionRemainSizeVectors_) {
            if (!vec.empty()) {
                uint32_t vecLength = vec.size() * sizeof(size_t);
                if (memcpy_s(writePtr, vecLength, vec.data(), vecLength) != EOK) {
                    LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memcpy_s write vec.data failed";
                    return false;
                }
                writePtr += vec.size() * sizeof(size_t);
            }
        }
    }
    // write buffer data
    if (data->bufferSize_ > 0) {
        if (!data->buffer_) {
            return false;
        }
        if (memcpy_s(writePtr, data->bufferSize_, data->buffer_, data->bufferSize_) != EOK) {
            LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile memcpy_s write buffer_ failed";
            return false;
        }
    }
    writePtr += data->bufferSize_;
    uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t checksum = adler32(0, reinterpret_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    if (memcpy_s(writePtr, checksumSize, &checksum, checksumSize) != EOK) {
        LOG_ECMA(ERROR) << "ModuleSnapshot::WriteDataToFile: checksum memcpy_s failed";
        return false;
    }
    FileSync(fileMapMem, FILE_MS_SYNC);
    LOG_ECMA(INFO) << "ModuleSnapshot::WriteDataToFile success";
    return true;
}
} // namespace panda::ecmascript
