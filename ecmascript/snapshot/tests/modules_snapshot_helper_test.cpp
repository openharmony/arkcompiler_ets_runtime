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

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/map.h"
#include "jsnapi_expo.h"
#include <string>
#include <string_view>
#include <fstream>

#define private public
#define protected public
#include "ecmascript/serializer/serialize_data.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#undef private
#undef protected

#include "gtest/gtest.h"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <system_error>

namespace panda::test {
using namespace panda::ecmascript;

class ModulesSnapshotHelperTest : public BaseTestWithScope<false> {
public:
    constexpr static const std::string_view MODULES_SNAPSHOT_FILE_DIR = "./";
    constexpr static const std::string_view EXISTING_TEST_FILE_NAME = "./test_module_snapshot_helper.txt";
    constexpr static const std::string_view NOT_EXISTING_TEST_FILE_NAME = "./non_existing_test_file.txt";
    void ResetSerializeData(std::unique_ptr<SerializeData>& data)
    {
        data->dataIndex_ = 0;
        data->sizeLimit_ = 0;
        data->bufferSize_ = 0;
        data->bufferCapacity_ = 0;
        data->regularSpaceSize_ = 0;
        data->pinSpaceSize_ = 0;
        data->oldSpaceSize_ = 0;
        data->nonMovableSpaceSize_ = 0;
        data->machineCodeSpaceSize_ = 0;
        data->sharedOldSpaceSize_ = 0;
        data->sharedNonMovableSpaceSize_ = 0;
        data->incompleteData_ = false;
    }
};

HWTEST_F_L0(ModulesSnapshotHelperTest, ModulesSnapshotHelper_SetReadOnly_Success)
{
    const std::string testFileName = "./test_module_snapshot_helper.txt";
    auto file = std::fopen(testFileName.c_str(), "w");
    ASSERT_NE(file, nullptr);
    std::fflush(file);
    std::string errorMsg{};
    bool result = ModulesSnapshotHelper::SetReadOnly(testFileName, &errorMsg);
    ASSERT_TRUE(result);
    ASSERT_TRUE(errorMsg.empty());
    std::fclose(file);
    std::remove(testFileName.c_str());
}

HWTEST_F_L0(ModulesSnapshotHelperTest, ModulesSnapshotHelper_SetReadOnly_FailedWithErrorMsg)
{
    std::string errorMsg{};
    ASSERT_FALSE(ModulesSnapshotHelper::SetReadOnly(NOT_EXISTING_TEST_FILE_NAME.data(), &errorMsg));
    ASSERT_FALSE(errorMsg.empty());
}

HWTEST_F_L0(ModulesSnapshotHelperTest, ModulesSnapshotHelper_SetReadOnly_FailedNoErrorMsg)
{
    ASSERT_FALSE(ModulesSnapshotHelper::SetReadOnly(NOT_EXISTING_TEST_FILE_NAME.data()));
}

HWTEST_F_L0(ModulesSnapshotHelperTest, ModulesSnapshotHelper_UpdateStateFromFile_NotOverrideDisabledFeature)
{
    auto originalFeatureState = ModulesSnapshotHelper::g_featureState_;
    auto originalFeatureLoaded = ModulesSnapshotHelper::g_featureLoaded_;
    ModulesSnapshotHelper::g_escaperTriggered_ = false;
    auto snapshotDir = CString(MODULES_SNAPSHOT_FILE_DIR);
    // init cached snapshot state file path
    ModulesSnapshotHelper::UpdateFromStateFile(snapshotDir);
    ModulesSnapshotHelper::g_featureState_ = static_cast<int>(SnapshotFeatureState::PANDAFILE);
    ModulesSnapshotHelper::g_featureLoaded_ =
        static_cast<int>(SnapshotFeatureState::PANDAFILE) | static_cast<int>(SnapshotFeatureState::MODULE);
    ModulesSnapshotHelper::TryDisableSnapshot(0);
    ModulesSnapshotHelper::UpdateFromStateFile(snapshotDir);
    EXPECT_TRUE(ModulesSnapshotHelper::IsPandafileSnapshotDisabled(snapshotDir));
    EXPECT_TRUE(ModulesSnapshotHelper::IsModuleSnapshotDisabled(snapshotDir));
    std::remove((snapshotDir + CString(ModulesSnapshotHelper::MODULE_SNAPSHOT_STATE_FILE_NAME)).c_str());
    ModulesSnapshotHelper::g_featureState_ = originalFeatureState;
    ModulesSnapshotHelper::g_featureLoaded_ = originalFeatureLoaded;
}

HWTEST_F_L0(ModulesSnapshotHelperTest, ModulesSnapshotHelper_UpdateStateFromFile_UpdateStateFileDir)
{
    std::string_view testFileDirA = "/";
    std::string_view testFileDirB = "./";
    std::string pathA = std::string(testFileDirA) + std::string(ModulesSnapshotHelper::MODULE_SNAPSHOT_STATE_FILE_NAME);
    std::string pathB = std::string(testFileDirB) + std::string(ModulesSnapshotHelper::MODULE_SNAPSHOT_STATE_FILE_NAME);
    ModulesSnapshotHelper::GetDisabledFeature(testFileDirA.data());
    ASSERT_EQ(std::string_view(ModulesSnapshotHelper::g_stateFilePathBuffer_), pathA);
    ModulesSnapshotHelper::GetDisabledFeature(testFileDirB.data());
    ASSERT_EQ(std::string_view(ModulesSnapshotHelper::g_stateFilePathBuffer_), pathB);
}

HWTEST_F_L0(ModulesSnapshotHelperTest, ModulesSnapshotHelper_TryDisableSnapshot_OnANR)
{
    auto originalFeatureState = ModulesSnapshotHelper::g_featureState_;
    auto originalFeatureLoaded = ModulesSnapshotHelper::g_featureLoaded_;
    ModulesSnapshotHelper::g_escaperTriggered_ = false;
    auto snapshotDir = CString(MODULES_SNAPSHOT_FILE_DIR);
    // init cached snapshot state file path
    ModulesSnapshotHelper::UpdateFromStateFile(snapshotDir);
    ModulesSnapshotHelper::g_featureState_ = static_cast<int>(SnapshotFeatureState::PANDAFILE);
    ModulesSnapshotHelper::g_featureLoaded_ =
        static_cast<int>(SnapshotFeatureState::PANDAFILE) | static_cast<int>(SnapshotFeatureState::MODULE);
    ModulesSnapshotHelper::TryDisableSnapshotOnANR();
    ModulesSnapshotHelper::UpdateFromStateFile(snapshotDir);
    EXPECT_TRUE(ModulesSnapshotHelper::IsPandafileSnapshotDisabled(snapshotDir));
    EXPECT_TRUE(ModulesSnapshotHelper::IsModuleSnapshotDisabled(snapshotDir));
    auto stateFilePath = base::ConcatToCString(snapshotDir, ModulesSnapshotHelper::MODULE_SNAPSHOT_STATE_FILE_NAME);
    std::ifstream ifs(stateFilePath.c_str());
    EXPECT_TRUE(ifs.is_open());
    std::string content;
    std::getline(ifs, content);
    EXPECT_TRUE(content.find(ModulesSnapshotHelper::DISABLE_APPLICATION_NOT_RESPONDING) != std::string::npos);
    ifs.close();
    std::remove(stateFilePath.c_str());
    ModulesSnapshotHelper::g_featureState_ = originalFeatureState;
    ModulesSnapshotHelper::g_featureLoaded_ = originalFeatureLoaded;
}

// FileGuard Tests
HWTEST_F_L0(ModulesSnapshotHelperTest, FileGuard_Constructor_GeneratesDifferentTempPath)
{
    const CString targetPath = "./test_fileguard_target.txt";
    uint32_t tid = 12345;
    ModulesSnapshotHelper::FileGuard fileGuard(targetPath, tid);

    // Temp path should be different from target path
    ASSERT_NE(fileGuard.GetTempPath(), targetPath);
    ASSERT_FALSE(fileGuard.GetTempPath().empty());
}

HWTEST_F_L0(ModulesSnapshotHelperTest, FileGuard_Done_SuccessWithExistingTempFile)
{
    const CString targetPath = "./test_fileguard_done_success.txt";

    // Ensure target file doesn't exist
    std::remove(targetPath.c_str());

    ModulesSnapshotHelper::FileGuard fileGuard(targetPath, 12345);
    CString tempPath = fileGuard.GetTempPath();

    // Temp path should be different from target path
    ASSERT_NE(tempPath, targetPath);
    ASSERT_FALSE(tempPath.empty());

    // Create temp file using ofstream
    std::ofstream ofs(tempPath.c_str());
    ASSERT_TRUE(ofs.is_open());
    ofs << "test content";
    ofs.close();

    bool result = fileGuard.Done();
    ASSERT_TRUE(result);

    // Verify target file exists and temp file is gone using FileExist
    ASSERT_TRUE(FileExist(targetPath.c_str()));
    ASSERT_FALSE(FileExist(tempPath.c_str()));

    // Cleanup
    std::remove(targetPath.c_str());
}

HWTEST_F_L0(ModulesSnapshotHelperTest, FileGuard_Done_FailsWhenTempFileNotExists)
{
    const CString targetPath = "./test_fileguard_done_fail.txt";

    // Ensure target file doesn't exist
    std::remove(targetPath.c_str());

    ModulesSnapshotHelper::FileGuard fileGuard(targetPath, 12345);
    CString tempPath = fileGuard.GetTempPath();

    // Ensure temp file doesn't exist
    std::remove(tempPath.c_str());

    bool result = fileGuard.Done();
    ASSERT_FALSE(result);

    // Verify target file doesn't exist using FileExist
    ASSERT_FALSE(FileExist(targetPath.c_str()));
}

HWTEST_F_L0(ModulesSnapshotHelperTest, FileGuard_Destructor_RemovesTempFileWhenNotDone)
{
    const CString targetPath = "./test_fileguard_destructor.txt";

    // Ensure target file doesn't exist
    std::remove(targetPath.c_str());

    CString tempPath;
    {
        ModulesSnapshotHelper::FileGuard fileGuard(targetPath, 12345);
        tempPath = fileGuard.GetTempPath();

        // Create temp file using ofstream
        std::ofstream ofs(tempPath.c_str());
        ASSERT_TRUE(ofs.is_open());
        ofs << "test content";
        ofs.close();
        // Don't call Done(), destructor should remove temp file
    }

    // Verify temp file is removed by destructor using FileExist
    ASSERT_FALSE(FileExist(tempPath.c_str()));

    // Verify target file doesn't exist using FileExist
    ASSERT_FALSE(FileExist(targetPath.c_str()));
}

HWTEST_F_L0(ModulesSnapshotHelperTest, FileGuard_Destructor_NotRemoveTargetFileWhenDone)
{
    const CString targetPath = "./test_fileguard_destructor_done.txt";

    // Ensure target file doesn't exist
    std::remove(targetPath.c_str());

    CString tempPath;
    {
        ModulesSnapshotHelper::FileGuard fileGuard(targetPath, 12345);
        tempPath = fileGuard.GetTempPath();

        // Create temp file using ofstream
        std::ofstream ofs(tempPath.c_str());
        ASSERT_TRUE(ofs.is_open());
        ofs << "test content";
        ofs.close();

        bool result = fileGuard.Done();
        ASSERT_TRUE(result);
        // After Done(), destructor should not remove target file
    }

    // Verify target file exists using FileExist
    ASSERT_TRUE(FileExist(targetPath.c_str()));

    // Cleanup
    std::remove(targetPath.c_str());
}

HWTEST_F_L0(ModulesSnapshotHelperTest, FileGuard_Done_Idempotent)
{
    const CString targetPath = "./test_fileguard_done_idempotent.txt";

    // Ensure target file doesn't exist
    std::remove(targetPath.c_str());

    ModulesSnapshotHelper::FileGuard fileGuard(targetPath, 12345);
    CString tempPath = fileGuard.GetTempPath();

    // Create temp file using ofstream
    std::ofstream ofs(tempPath.c_str());
    ASSERT_TRUE(ofs.is_open());
    ofs << "test content";
    ofs.close();

    // Call Done() multiple times
    bool result1 = fileGuard.Done();
    ASSERT_TRUE(result1);

    bool result2 = fileGuard.Done();
    ASSERT_TRUE(result2);

    bool result3 = fileGuard.Done();
    ASSERT_TRUE(result3);

    // Verify target file exists using FileExist
    ASSERT_TRUE(FileExist(targetPath.c_str()));

    // Cleanup
    std::remove(targetPath.c_str());
}

// WriteDataInfo Tests
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteDataInfo_Success)
{
    // Prepare SerializeData with test values
    auto data = std::make_unique<SerializeData>(thread);
    data->dataIndex_ = 42;
    data->sizeLimit_ = 1024;
    data->bufferSize_ = 512;
    data->bufferCapacity_ = 1024;
    data->regularSpaceSize_ = 256;
    data->pinSpaceSize_ = 64;
    data->oldSpaceSize_ = 128;
    data->nonMovableSpaceSize_ = 32;
    data->machineCodeSpaceSize_ = 16;
    data->sharedOldSpaceSize_ = 48;
    data->sharedNonMovableSpaceSize_ = 24;
    data->incompleteData_ = false;

    // Allocate buffer large enough for SerializeDataInfo
    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 64; // extra padding
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteDataInfo_Success");

    bool result = ModulesSnapshotHelper::WriteDataInfo(writer, data);
    ASSERT_TRUE(result);

    // Verify the written data by reading it back
    auto writtenInfo = reinterpret_cast<const ModulesSnapshotHelper::SerializeDataInfo*>(buffer.data());
    EXPECT_EQ(writtenInfo->dataIndex_, data->dataIndex_);
    EXPECT_EQ(writtenInfo->sizeLimit_, data->sizeLimit_);
    EXPECT_EQ(writtenInfo->bufferSize_, data->bufferSize_);
    EXPECT_EQ(writtenInfo->bufferCapacity_, data->bufferCapacity_);
    EXPECT_EQ(writtenInfo->regularSpaceSize_, data->regularSpaceSize_);
    EXPECT_EQ(writtenInfo->pinSpaceSize_, data->pinSpaceSize_);
    EXPECT_EQ(writtenInfo->oldSpaceSize_, data->oldSpaceSize_);
    EXPECT_EQ(writtenInfo->nonMovableSpaceSize_, data->nonMovableSpaceSize_);
    EXPECT_EQ(writtenInfo->machineCodeSpaceSize_, data->machineCodeSpaceSize_);
    EXPECT_EQ(writtenInfo->sharedOldSpaceSize_, data->sharedOldSpaceSize_);
    EXPECT_EQ(writtenInfo->sharedNonMovableSpaceSize_, data->sharedNonMovableSpaceSize_);
    EXPECT_EQ(writtenInfo->incompleteData_, data->incompleteData_);
    ResetSerializeData(data);
}

HWTEST_F_L0(ModulesSnapshotHelperTest, WriteDataInfo_WithIncompleteDataTrue)
{
    auto data = std::make_unique<SerializeData>(thread);
    data->dataIndex_ = 100;
    data->sizeLimit_ = 2048;
    data->bufferSize_ = 1024;
    data->bufferCapacity_ = 2048;
    data->regularSpaceSize_ = 512;
    data->pinSpaceSize_ = 128;
    data->oldSpaceSize_ = 256;
    data->nonMovableSpaceSize_ = 64;
    data->machineCodeSpaceSize_ = 32;
    data->sharedOldSpaceSize_ = 96;
    data->sharedNonMovableSpaceSize_ = 48;
    data->incompleteData_ = true;

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteDataInfo_IncompleteData");

    bool result = ModulesSnapshotHelper::WriteDataInfo(writer, data);
    ASSERT_TRUE(result);

    auto writtenInfo = reinterpret_cast<const ModulesSnapshotHelper::SerializeDataInfo*>(buffer.data());
    EXPECT_EQ(writtenInfo->dataIndex_, 100u);
    EXPECT_EQ(writtenInfo->sizeLimit_, 2048ull);
    EXPECT_EQ(writtenInfo->incompleteData_, true);
}

HWTEST_F_L0(ModulesSnapshotHelperTest, WriteDataInfo_WithZeroValues)
{
    auto data = std::make_unique<SerializeData>(thread);
    data->dataIndex_ = 0;
    data->sizeLimit_ = 0;
    data->bufferSize_ = 0;
    data->bufferCapacity_ = 0;
    data->regularSpaceSize_ = 0;
    data->pinSpaceSize_ = 0;
    data->oldSpaceSize_ = 0;
    data->nonMovableSpaceSize_ = 0;
    data->machineCodeSpaceSize_ = 0;
    data->sharedOldSpaceSize_ = 0;
    data->sharedNonMovableSpaceSize_ = 0;
    data->incompleteData_ = false;

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteDataInfo_ZeroValues");

    bool result = ModulesSnapshotHelper::WriteDataInfo(writer, data);
    ASSERT_TRUE(result);

    auto writtenInfo = reinterpret_cast<const ModulesSnapshotHelper::SerializeDataInfo*>(buffer.data());
    EXPECT_EQ(writtenInfo->dataIndex_, 0u);
    EXPECT_EQ(writtenInfo->sizeLimit_, 0ull);
    EXPECT_EQ(writtenInfo->bufferSize_, static_cast<size_t>(0));
    EXPECT_EQ(writtenInfo->incompleteData_, false);
}

// ReadDataInfo Tests
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadDataInfo_Success)
{
    // Prepare SerializeDataInfo with test values and write into buffer
    ModulesSnapshotHelper::SerializeDataInfo writeInfo {
        .dataIndex_ = 42,
        .sizeLimit_ = 1024,
        .bufferSize_ = 512,
        .bufferCapacity_ = 1024,
        .regularSpaceSize_ = 256,
        .pinSpaceSize_ = 64,
        .oldSpaceSize_ = 128,
        .nonMovableSpaceSize_ = 32,
        .machineCodeSpaceSize_ = 16,
        .sharedOldSpaceSize_ = 48,
        .sharedNonMovableSpaceSize_ = 24,
        .incompleteData_ = false,
    };

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    memcpy(buffer.data(), &writeInfo, sizeof(writeInfo));

    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapReader reader(memMap, []() {}, "ReadDataInfo_Success");

    auto data = std::make_unique<SerializeData>(thread);
    bool result = ModulesSnapshotHelper::ReadDataInfo(reader, data);
    ASSERT_TRUE(result);

    EXPECT_EQ(data->dataIndex_, writeInfo.dataIndex_);
    EXPECT_EQ(data->sizeLimit_, writeInfo.sizeLimit_);
    EXPECT_EQ(data->bufferSize_, writeInfo.bufferSize_);
    EXPECT_EQ(data->bufferCapacity_, writeInfo.bufferCapacity_);
    EXPECT_EQ(data->regularSpaceSize_, writeInfo.regularSpaceSize_);
    EXPECT_EQ(data->pinSpaceSize_, writeInfo.pinSpaceSize_);
    EXPECT_EQ(data->oldSpaceSize_, writeInfo.oldSpaceSize_);
    EXPECT_EQ(data->nonMovableSpaceSize_, writeInfo.nonMovableSpaceSize_);
    EXPECT_EQ(data->machineCodeSpaceSize_, writeInfo.machineCodeSpaceSize_);
    EXPECT_EQ(data->sharedOldSpaceSize_, writeInfo.sharedOldSpaceSize_);
    EXPECT_EQ(data->sharedNonMovableSpaceSize_, writeInfo.sharedNonMovableSpaceSize_);
    EXPECT_EQ(data->incompleteData_, writeInfo.incompleteData_);
    ResetSerializeData(data);
}

HWTEST_F_L0(ModulesSnapshotHelperTest, ReadDataInfo_WithIncompleteDataTrue)
{
    ModulesSnapshotHelper::SerializeDataInfo writeInfo {
        .dataIndex_ = 100,
        .sizeLimit_ = 2048,
        .bufferSize_ = 1024,
        .bufferCapacity_ = 2048,
        .regularSpaceSize_ = 512,
        .pinSpaceSize_ = 128,
        .oldSpaceSize_ = 256,
        .nonMovableSpaceSize_ = 64,
        .machineCodeSpaceSize_ = 32,
        .sharedOldSpaceSize_ = 96,
        .sharedNonMovableSpaceSize_ = 48,
        .incompleteData_ = true,
    };

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    memcpy(buffer.data(), &writeInfo, sizeof(writeInfo));

    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapReader reader(memMap, []() {}, "ReadDataInfo_IncompleteData");

    auto data = std::make_unique<SerializeData>(thread);
    bool result = ModulesSnapshotHelper::ReadDataInfo(reader, data);
    ASSERT_TRUE(result);

    EXPECT_EQ(data->dataIndex_, 100u);
    EXPECT_EQ(data->sizeLimit_, 2048ull);
    EXPECT_EQ(data->incompleteData_, true);
}

HWTEST_F_L0(ModulesSnapshotHelperTest, ReadDataInfo_FailsWithInsufficientRemainingSize)
{
    // Use a buffer with only 1 byte - way less than sizeof(SerializeDataInfo)
    std::vector<uint8_t> buffer(1, 0);
    MemMap memMap(buffer.data(), 1);
    FileMemMapReader reader(memMap, []() {}, "ReadDataInfo_Fail");

    auto data = std::make_unique<SerializeData>(thread);
    bool result = ModulesSnapshotHelper::ReadDataInfo(reader, data);
    // Step() should fail because remaining size is less than sizeof(SerializeDataInfo)
    ASSERT_FALSE(result);
}

// Write-then-Read Roundtrip Tests
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteAndReadDataInfo_Roundtrip)
{
    auto writeData = std::make_unique<SerializeData>(thread);
    writeData->dataIndex_ = 99;
    writeData->sizeLimit_ = 4096;
    writeData->bufferSize_ = 2048;
    writeData->bufferCapacity_ = 4096;
    writeData->regularSpaceSize_ = 1024;
    writeData->pinSpaceSize_ = 256;
    writeData->oldSpaceSize_ = 512;
    writeData->nonMovableSpaceSize_ = 128;
    writeData->machineCodeSpaceSize_ = 64;
    writeData->sharedOldSpaceSize_ = 192;
    writeData->sharedNonMovableSpaceSize_ = 96;
    writeData->incompleteData_ = true;

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 128;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap writeMemMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(writeMemMap, "Roundtrip_Write");

    bool writeResult = ModulesSnapshotHelper::WriteDataInfo(writer, writeData);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "Roundtrip_Read");

    auto readData = std::make_unique<SerializeData>(thread);
    bool readResult = ModulesSnapshotHelper::ReadDataInfo(reader, readData);
    ASSERT_TRUE(readResult);

    // Verify all fields match after roundtrip
    EXPECT_EQ(readData->dataIndex_, writeData->dataIndex_);
    EXPECT_EQ(readData->sizeLimit_, writeData->sizeLimit_);
    EXPECT_EQ(readData->bufferSize_, writeData->bufferSize_);
    EXPECT_EQ(readData->bufferCapacity_, writeData->bufferCapacity_);
    EXPECT_EQ(readData->regularSpaceSize_, writeData->regularSpaceSize_);
    EXPECT_EQ(readData->pinSpaceSize_, writeData->pinSpaceSize_);
    EXPECT_EQ(readData->oldSpaceSize_, writeData->oldSpaceSize_);
    EXPECT_EQ(readData->nonMovableSpaceSize_, writeData->nonMovableSpaceSize_);
    EXPECT_EQ(readData->machineCodeSpaceSize_, writeData->machineCodeSpaceSize_);
    EXPECT_EQ(readData->sharedOldSpaceSize_, writeData->sharedOldSpaceSize_);
    EXPECT_EQ(readData->sharedNonMovableSpaceSize_, writeData->sharedNonMovableSpaceSize_);
    EXPECT_EQ(readData->incompleteData_, writeData->incompleteData_);
}

HWTEST_F_L0(ModulesSnapshotHelperTest, WriteAndReadDataInfo_RoundtripWithZeroValues)
{
    auto writeData = std::make_unique<SerializeData>(thread);
    writeData->dataIndex_ = 0;
    writeData->sizeLimit_ = 0;
    writeData->bufferSize_ = 0;
    writeData->bufferCapacity_ = 0;
    writeData->regularSpaceSize_ = 0;
    writeData->pinSpaceSize_ = 0;
    writeData->oldSpaceSize_ = 0;
    writeData->nonMovableSpaceSize_ = 0;
    writeData->machineCodeSpaceSize_ = 0;
    writeData->sharedOldSpaceSize_ = 0;
    writeData->sharedNonMovableSpaceSize_ = 0;
    writeData->incompleteData_ = false;

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 128;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap writeMemMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(writeMemMap, "RoundtripZero_Write");

    bool writeResult = ModulesSnapshotHelper::WriteDataInfo(writer, writeData);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "RoundtripZero_Read");

    auto readData = std::make_unique<SerializeData>(thread);
    bool readResult = ModulesSnapshotHelper::ReadDataInfo(reader, readData);
    ASSERT_TRUE(readResult);

    // Verify all fields are zero/default after roundtrip
    EXPECT_EQ(readData->dataIndex_, 0u);
    EXPECT_EQ(readData->sizeLimit_, 0ull);
    EXPECT_EQ(readData->bufferSize_, static_cast<size_t>(0));
    EXPECT_EQ(readData->bufferCapacity_, static_cast<size_t>(0));
    EXPECT_EQ(readData->regularSpaceSize_, static_cast<size_t>(0));
    EXPECT_EQ(readData->pinSpaceSize_, static_cast<size_t>(0));
    EXPECT_EQ(readData->oldSpaceSize_, static_cast<size_t>(0));
    EXPECT_EQ(readData->nonMovableSpaceSize_, static_cast<size_t>(0));
    EXPECT_EQ(readData->machineCodeSpaceSize_, static_cast<size_t>(0));
    EXPECT_EQ(readData->sharedOldSpaceSize_, static_cast<size_t>(0));
    EXPECT_EQ(readData->sharedNonMovableSpaceSize_, static_cast<size_t>(0));
    EXPECT_EQ(readData->incompleteData_, false);
}

// WriteDataInfo: write pointer advancement verification
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteDataInfo_AdvancesWritePointer)
{
    auto data = std::make_unique<SerializeData>(thread);
    data->dataIndex_ = 7;
    data->sizeLimit_ = 100;
    data->bufferSize_ = 50;
    data->bufferCapacity_ = 100;
    data->regularSpaceSize_ = 20;
    data->pinSpaceSize_ = 10;
    data->oldSpaceSize_ = 15;
    data->nonMovableSpaceSize_ = 5;
    data->machineCodeSpaceSize_ = 3;
    data->sharedOldSpaceSize_ = 8;
    data->sharedNonMovableSpaceSize_ = 4;
    data->incompleteData_ = false;

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 128;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteDataInfo_AdvancesWritePointer");

    uint8_t *writePtrBefore = writer.GetWritePtr();
    bool result = ModulesSnapshotHelper::WriteDataInfo(writer, data);
    ASSERT_TRUE(result);
    uint8_t *writePtrAfter = writer.GetWritePtr();

    // Verify that the write pointer has advanced by exactly sizeof(SerializeDataInfo)
    EXPECT_EQ(static_cast<size_t>(writePtrAfter - writePtrBefore), sizeof(ModulesSnapshotHelper::SerializeDataInfo));
    ResetSerializeData(data);
}

// WriteDataInfo: buffer sized exactly sizeof(SerializeDataInfo)
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteDataInfo_BufferExactlySized)
{
    auto data = std::make_unique<SerializeData>(thread);
    data->dataIndex_ = 1;
    data->sizeLimit_ = 64;
    data->bufferSize_ = 32;
    data->bufferCapacity_ = 64;
    data->regularSpaceSize_ = 16;
    data->pinSpaceSize_ = 4;
    data->oldSpaceSize_ = 8;
    data->nonMovableSpaceSize_ = 2;
    data->machineCodeSpaceSize_ = 1;
    data->sharedOldSpaceSize_ = 4;
    data->sharedNonMovableSpaceSize_ = 2;
    data->incompleteData_ = false;

    // Buffer of exactly sizeof(SerializeDataInfo) — no extra padding
    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo);
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteDataInfo_BufferExactlySized");

    bool result = ModulesSnapshotHelper::WriteDataInfo(writer, data);
    ASSERT_TRUE(result);
    ResetSerializeData(data);
}

// WriteDataInfo: failure when buffer is too small for memcpy_s
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteDataInfo_FailsWithBufferTooSmall)
{
    auto data = std::make_unique<SerializeData>(thread);
    data->dataIndex_ = 1;
    data->sizeLimit_ = 0;
    data->bufferSize_ = 0;
    data->bufferCapacity_ = 0;
    data->regularSpaceSize_ = 0;
    data->pinSpaceSize_ = 0;
    data->oldSpaceSize_ = 0;
    data->nonMovableSpaceSize_ = 0;
    data->machineCodeSpaceSize_ = 0;
    data->sharedOldSpaceSize_ = 0;
    data->sharedNonMovableSpaceSize_ = 0;
    data->incompleteData_ = false;

    // Buffer far too small — memcpy_s should fail since writeSize > fileSize
    size_t bufferSize = 1;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteDataInfo_FailBufferTooSmall");

    bool result = ModulesSnapshotHelper::WriteDataInfo(writer, data);
    ASSERT_FALSE(result);
    ResetSerializeData(data);
}

// WriteDataInfo: large numeric boundary values
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteDataInfo_WithMaxNumericValues)
{
    auto data = std::make_unique<SerializeData>(thread);
    data->dataIndex_ = UINT32_MAX;
    data->sizeLimit_ = UINT64_MAX;
    data->bufferSize_ = SIZE_MAX;
    data->bufferCapacity_ = SIZE_MAX;
    data->regularSpaceSize_ = SIZE_MAX;
    data->pinSpaceSize_ = SIZE_MAX;
    data->oldSpaceSize_ = SIZE_MAX;
    data->nonMovableSpaceSize_ = SIZE_MAX;
    data->machineCodeSpaceSize_ = SIZE_MAX;
    data->sharedOldSpaceSize_ = SIZE_MAX;
    data->sharedNonMovableSpaceSize_ = SIZE_MAX;
    data->incompleteData_ = true;

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteDataInfo_MaxNumericValues");

    bool result = ModulesSnapshotHelper::WriteDataInfo(writer, data);
    ASSERT_TRUE(result);

    auto writtenInfo = reinterpret_cast<const ModulesSnapshotHelper::SerializeDataInfo*>(buffer.data());
    EXPECT_EQ(writtenInfo->dataIndex_, UINT32_MAX);
    EXPECT_EQ(writtenInfo->sizeLimit_, UINT64_MAX);
    EXPECT_EQ(writtenInfo->bufferSize_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->bufferCapacity_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->regularSpaceSize_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->pinSpaceSize_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->oldSpaceSize_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->nonMovableSpaceSize_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->machineCodeSpaceSize_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->sharedOldSpaceSize_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->sharedNonMovableSpaceSize_, SIZE_MAX);
    EXPECT_EQ(writtenInfo->incompleteData_, true);
    ResetSerializeData(data);
}

// ReadDataInfo: read pointer advancement verification
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadDataInfo_AdvancesReadPointer)
{
    ModulesSnapshotHelper::SerializeDataInfo writeInfo {
        .dataIndex_ = 7,
        .sizeLimit_ = 100,
        .bufferSize_ = 50,
        .bufferCapacity_ = 100,
        .regularSpaceSize_ = 20,
        .pinSpaceSize_ = 10,
        .oldSpaceSize_ = 15,
        .nonMovableSpaceSize_ = 5,
        .machineCodeSpaceSize_ = 3,
        .sharedOldSpaceSize_ = 8,
        .sharedNonMovableSpaceSize_ = 4,
        .incompleteData_ = false,
    };

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 128;
    std::vector<uint8_t> buffer(bufferSize, 0);
    memcpy(buffer.data(), &writeInfo, sizeof(writeInfo));

    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapReader reader(memMap, []() {}, "ReadDataInfo_AdvancesReadPointer");

    uint8_t *readPtrBefore = reader.GetReadPtr();
    auto data = std::make_unique<SerializeData>(thread);
    bool result = ModulesSnapshotHelper::ReadDataInfo(reader, data);
    ASSERT_TRUE(result);
    uint8_t *readPtrAfter = reader.GetReadPtr();

    // Verify that the read pointer has advanced by exactly sizeof(SerializeDataInfo)
    EXPECT_EQ(static_cast<size_t>(readPtrAfter - readPtrBefore), sizeof(ModulesSnapshotHelper::SerializeDataInfo));
    ResetSerializeData(data);
}

// ReadDataInfo: buffer sized exactly sizeof(SerializeDataInfo)
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadDataInfo_BufferExactlySized)
{
    ModulesSnapshotHelper::SerializeDataInfo writeInfo {
        .dataIndex_ = 1,
        .sizeLimit_ = 64,
        .bufferSize_ = 32,
        .bufferCapacity_ = 64,
        .regularSpaceSize_ = 16,
        .pinSpaceSize_ = 4,
        .oldSpaceSize_ = 8,
        .nonMovableSpaceSize_ = 2,
        .machineCodeSpaceSize_ = 1,
        .sharedOldSpaceSize_ = 4,
        .sharedNonMovableSpaceSize_ = 2,
        .incompleteData_ = false,
    };

    // Buffer of exactly sizeof(SerializeDataInfo) — no extra padding
    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo);
    std::vector<uint8_t> buffer(bufferSize, 0);
    memcpy(buffer.data(), &writeInfo, sizeof(writeInfo));

    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapReader reader(memMap, []() {}, "ReadDataInfo_BufferExactlySized");

    auto data = std::make_unique<SerializeData>(thread);
    bool result = ModulesSnapshotHelper::ReadDataInfo(reader, data);
    ASSERT_TRUE(result);

    EXPECT_EQ(data->dataIndex_, 1u);
    EXPECT_EQ(data->sizeLimit_, 64ull);
    EXPECT_EQ(data->bufferSize_, static_cast<size_t>(32));
    EXPECT_EQ(data->bufferCapacity_, static_cast<size_t>(64));
    EXPECT_EQ(data->regularSpaceSize_, static_cast<size_t>(16));
    EXPECT_EQ(data->pinSpaceSize_, static_cast<size_t>(4));
    EXPECT_EQ(data->oldSpaceSize_, static_cast<size_t>(8));
    EXPECT_EQ(data->nonMovableSpaceSize_, static_cast<size_t>(2));
    EXPECT_EQ(data->machineCodeSpaceSize_, static_cast<size_t>(1));
    EXPECT_EQ(data->sharedOldSpaceSize_, static_cast<size_t>(4));
    EXPECT_EQ(data->sharedNonMovableSpaceSize_, static_cast<size_t>(2));
    EXPECT_EQ(data->incompleteData_, false);
    ResetSerializeData(data);
}

// ReadDataInfo: failure does not modify pre-existing SerializeData fields
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadDataInfo_FailurePreservesExistingData)
{
    // Create a SerializeData with known pre-existing values
    auto data = std::make_unique<SerializeData>(thread);
    data->dataIndex_ = 999;
    data->sizeLimit_ = 888;
    data->bufferSize_ = 777;
    data->bufferCapacity_ = 666;
    data->regularSpaceSize_ = 555;
    data->pinSpaceSize_ = 444;
    data->oldSpaceSize_ = 333;
    data->nonMovableSpaceSize_ = 222;
    data->machineCodeSpaceSize_ = 111;
    data->sharedOldSpaceSize_ = 100;
    data->sharedNonMovableSpaceSize_ = 50;
    data->incompleteData_ = true;

    // Use a buffer far too small so ReadDataInfo will fail on Step()
    std::vector<uint8_t> buffer(1, 0);
    MemMap memMap(buffer.data(), 1);
    FileMemMapReader reader(memMap, []() {}, "ReadDataInfo_FailurePreservesExistingData");

    bool result = ModulesSnapshotHelper::ReadDataInfo(reader, data);
    ASSERT_FALSE(result);

    // Verify that all pre-existing fields remain unchanged after failure
    EXPECT_EQ(data->dataIndex_, 999u);
    EXPECT_EQ(data->sizeLimit_, 888ull);
    EXPECT_EQ(data->bufferSize_, static_cast<size_t>(777));
    EXPECT_EQ(data->bufferCapacity_, static_cast<size_t>(666));
    EXPECT_EQ(data->regularSpaceSize_, static_cast<size_t>(555));
    EXPECT_EQ(data->pinSpaceSize_, static_cast<size_t>(444));
    EXPECT_EQ(data->oldSpaceSize_, static_cast<size_t>(333));
    EXPECT_EQ(data->nonMovableSpaceSize_, static_cast<size_t>(222));
    EXPECT_EQ(data->machineCodeSpaceSize_, static_cast<size_t>(111));
    EXPECT_EQ(data->sharedOldSpaceSize_, static_cast<size_t>(100));
    EXPECT_EQ(data->sharedNonMovableSpaceSize_, static_cast<size_t>(50));
    EXPECT_EQ(data->incompleteData_, true);
    ResetSerializeData(data);
}

// ReadDataInfo: large numeric boundary values roundtrip
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadDataInfo_WithMaxNumericValues)
{
    ModulesSnapshotHelper::SerializeDataInfo writeInfo {
        .dataIndex_ = UINT32_MAX,
        .sizeLimit_ = UINT64_MAX,
        .bufferSize_ = SIZE_MAX,
        .bufferCapacity_ = SIZE_MAX,
        .regularSpaceSize_ = SIZE_MAX,
        .pinSpaceSize_ = SIZE_MAX,
        .oldSpaceSize_ = SIZE_MAX,
        .nonMovableSpaceSize_ = SIZE_MAX,
        .machineCodeSpaceSize_ = SIZE_MAX,
        .sharedOldSpaceSize_ = SIZE_MAX,
        .sharedNonMovableSpaceSize_ = SIZE_MAX,
        .incompleteData_ = true,
    };

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    memcpy(buffer.data(), &writeInfo, sizeof(writeInfo));

    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapReader reader(memMap, []() {}, "ReadDataInfo_MaxNumericValues");

    auto data = std::make_unique<SerializeData>(thread);
    bool result = ModulesSnapshotHelper::ReadDataInfo(reader, data);
    ASSERT_TRUE(result);

    EXPECT_EQ(data->dataIndex_, UINT32_MAX);
    EXPECT_EQ(data->sizeLimit_, UINT64_MAX);
    EXPECT_EQ(data->bufferSize_, SIZE_MAX);
    EXPECT_EQ(data->bufferCapacity_, SIZE_MAX);
    EXPECT_EQ(data->regularSpaceSize_, SIZE_MAX);
    EXPECT_EQ(data->pinSpaceSize_, SIZE_MAX);
    EXPECT_EQ(data->oldSpaceSize_, SIZE_MAX);
    EXPECT_EQ(data->nonMovableSpaceSize_, SIZE_MAX);
    EXPECT_EQ(data->machineCodeSpaceSize_, SIZE_MAX);
    EXPECT_EQ(data->sharedOldSpaceSize_, SIZE_MAX);
    EXPECT_EQ(data->sharedNonMovableSpaceSize_, SIZE_MAX);
    EXPECT_EQ(data->incompleteData_, true);
    ResetSerializeData(data);
}

// Roundtrip with max numeric values
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteAndReadDataInfo_RoundtripWithMaxValues)
{
    auto writeData = std::make_unique<SerializeData>(thread);
    writeData->dataIndex_ = UINT32_MAX;
    writeData->sizeLimit_ = UINT64_MAX;
    writeData->bufferSize_ = SIZE_MAX;
    writeData->bufferCapacity_ = SIZE_MAX;
    writeData->regularSpaceSize_ = SIZE_MAX;
    writeData->pinSpaceSize_ = SIZE_MAX;
    writeData->oldSpaceSize_ = SIZE_MAX;
    writeData->nonMovableSpaceSize_ = SIZE_MAX;
    writeData->machineCodeSpaceSize_ = SIZE_MAX;
    writeData->sharedOldSpaceSize_ = SIZE_MAX;
    writeData->sharedNonMovableSpaceSize_ = SIZE_MAX;
    writeData->incompleteData_ = true;

    size_t bufferSize = sizeof(ModulesSnapshotHelper::SerializeDataInfo) + 128;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap writeMemMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(writeMemMap, "RoundtripMax_Write");

    bool writeResult = ModulesSnapshotHelper::WriteDataInfo(writer, writeData);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "RoundtripMax_Read");

    auto readData = std::make_unique<SerializeData>(thread);
    bool readResult = ModulesSnapshotHelper::ReadDataInfo(reader, readData);
    ASSERT_TRUE(readResult);

    EXPECT_EQ(readData->dataIndex_, UINT32_MAX);
    EXPECT_EQ(readData->sizeLimit_, UINT64_MAX);
    EXPECT_EQ(readData->bufferSize_, SIZE_MAX);
    EXPECT_EQ(readData->bufferCapacity_, SIZE_MAX);
    EXPECT_EQ(readData->regularSpaceSize_, SIZE_MAX);
    EXPECT_EQ(readData->pinSpaceSize_, SIZE_MAX);
    EXPECT_EQ(readData->oldSpaceSize_, SIZE_MAX);
    EXPECT_EQ(readData->nonMovableSpaceSize_, SIZE_MAX);
    EXPECT_EQ(readData->machineCodeSpaceSize_, SIZE_MAX);
    EXPECT_EQ(readData->sharedOldSpaceSize_, SIZE_MAX);
    EXPECT_EQ(readData->sharedNonMovableSpaceSize_, SIZE_MAX);
    EXPECT_EQ(readData->incompleteData_, true);
    ResetSerializeData(readData);
}

// ==================== WriteFileHeader / ReadFileHeader Tests ====================

// WriteFileHeader: success with typical values
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteFileHeader_Success)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(10, "5.0.1", "ArkTS-Snapshots");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size() + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteFileHeader_Success");

    bool result = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(result);

    // Verify the written data matches the original header
    auto writtenHeader = ModulesSnapshotHelper::SnapshotVersionInfo::FromBuffer(buffer.data());
    EXPECT_EQ(writtenHeader->appVersionCode_, header->appVersionCode_);
    EXPECT_EQ(writtenHeader->romVerLength_, header->romVerLength_);
    EXPECT_EQ(writtenHeader->descLength_, header->descLength_);
    EXPECT_EQ(writtenHeader->GetRomVersion(), header->GetRomVersion());
    EXPECT_EQ(writtenHeader->GetDescription(), header->GetDescription());
}

// WriteFileHeader: advances write pointer by header->Size()
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteFileHeader_AdvancesWritePointer)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(1, "ROM", "desc");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size() + 128;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteFileHeader_AdvancesWritePointer");

    uint8_t *writePtrBefore = writer.GetWritePtr();
    bool result = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(result);
    uint8_t *writePtrAfter = writer.GetWritePtr();

    EXPECT_EQ(static_cast<size_t>(writePtrAfter - writePtrBefore), header->Size());
}

// WriteFileHeader: buffer exactly sized to header->Size()
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteFileHeader_BufferExactlySized)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(5, "RomVer", "DescInfo");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size();
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteFileHeader_BufferExactlySized");

    bool result = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(result);
}

// WriteFileHeader: failure when buffer is too small
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteFileHeader_FailsWithBufferTooSmall)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(1, "R", "D");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = 1;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteFileHeader_FailBufferTooSmall");

    bool result = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_FALSE(result);
}

// WriteFileHeader: empty romVersion and description
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteFileHeader_EmptyStrings)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(0, "", "");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size() + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "WriteFileHeader_EmptyStrings");

    bool result = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(result);

    auto writtenHeader = ModulesSnapshotHelper::SnapshotVersionInfo::FromBuffer(buffer.data());
    EXPECT_EQ(writtenHeader->appVersionCode_, 0u);
    EXPECT_EQ(writtenHeader->romVerLength_, static_cast<size_t>(0));
    EXPECT_EQ(writtenHeader->descLength_, static_cast<size_t>(0));
    EXPECT_EQ(writtenHeader->GetRomVersion(), std::string_view(""));
    EXPECT_EQ(writtenHeader->GetDescription(), std::string_view(""));
}

// ReadFileHeader: success when all fields match
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadFileHeader_SuccessAllMatch)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(10, "5.0.1", "ArkTS-Snapshots");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size() + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "ReadFileHeader_WriteForRead");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "ReadFileHeader_SuccessAllMatch");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, header);
    EXPECT_TRUE(result.empty());
}

// ReadFileHeader: advances read pointer by header->Size()
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadFileHeader_AdvancesReadPointer)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(1, "ROM", "desc");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size() + 128;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "ReadFileHeader_AdvancesWrite");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "ReadFileHeader_AdvancesReadPointer");

    // Record the header size from the buffer before reading
    auto mHeader = ModulesSnapshotHelper::SnapshotVersionInfo::FromBuffer(buffer.data());
    size_t expectedStepSize = mHeader->Size();

    uint8_t *readPtrBefore = reader.GetReadPtr();
    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, header);
    EXPECT_TRUE(result.empty());
    uint8_t *readPtrAfter = reader.GetReadPtr();

    EXPECT_EQ(static_cast<size_t>(readPtrAfter - readPtrBefore), expectedStepSize);
}

// ReadFileHeader: failure with insufficient remaining size
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadFileHeader_FailsWithInsufficientSize)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(1, "R", "D");
    ASSERT_NE(header, nullptr);

    // Buffer too small for Step to succeed
    std::vector<uint8_t> buffer(1, 0);
    MemMap memMap(buffer.data(), 1);
    FileMemMapReader reader(memMap, []() {}, "ReadFileHeader_FailInsufficient");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, header);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("failed"), std::string::npos);
}

// ReadFileHeader: appVersionCode mismatch
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadFileHeader_AppVersionCodeMismatch)
{
    auto writeHeader = ModulesSnapshotHelper::SnapshotVersionInfo::New(10, "5.0.1", "desc");
    ASSERT_NE(writeHeader, nullptr);
    auto readHeader = ModulesSnapshotHelper::SnapshotVersionInfo::New(20, "5.0.1", "desc");
    ASSERT_NE(readHeader, nullptr);

    size_t bufferSize = writeHeader->Size() + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "ReadFileHeader_WriteMismatch");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, writeHeader);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "ReadFileHeader_AppVersionCodeMismatch");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, readHeader);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("application version code"), std::string::npos);
    EXPECT_NE(result.find("mismatch"), std::string::npos);
    EXPECT_NE(result.find("10"), std::string::npos); // read value
    EXPECT_NE(result.find("20"), std::string::npos); // current value
}

// ReadFileHeader: romVersion mismatch
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadFileHeader_RomVersionMismatch)
{
    auto writeHeader = ModulesSnapshotHelper::SnapshotVersionInfo::New(10, "5.0.1", "desc");
    ASSERT_NE(writeHeader, nullptr);
    auto readHeader = ModulesSnapshotHelper::SnapshotVersionInfo::New(10, "5.0.2", "desc");
    ASSERT_NE(readHeader, nullptr);

    size_t bufferSize = writeHeader->Size() + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "ReadFileHeader_WriteMismatch");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, writeHeader);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "ReadFileHeader_RomVersionMismatch");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, readHeader);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("ROM version"), std::string::npos);
    EXPECT_NE(result.find("mismatch"), std::string::npos);
    EXPECT_NE(result.find("5.0.1"), std::string::npos); // read value from buffer
    EXPECT_NE(result.find("5.0.2"), std::string::npos); // current value from readHeader
}

// ReadFileHeader: description mismatch
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadFileHeader_DescriptionMismatch)
{
    auto writeHeader = ModulesSnapshotHelper::SnapshotVersionInfo::New(10, "5.0.1", "desc-A");
    ASSERT_NE(writeHeader, nullptr);
    auto readHeader = ModulesSnapshotHelper::SnapshotVersionInfo::New(10, "5.0.1", "desc-B");
    ASSERT_NE(readHeader, nullptr);

    size_t bufferSize = writeHeader->Size() + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "ReadFileHeader_WriteMismatch");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, writeHeader);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "ReadFileHeader_DescriptionMismatch");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, readHeader);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("snapshot file description"), std::string::npos);
    EXPECT_NE(result.find("mismatch"), std::string::npos);
    EXPECT_NE(result.find("desc-A"), std::string::npos); // read value from buffer
    EXPECT_NE(result.find("desc-B"), std::string::npos); // current value from readHeader
}

// ReadFileHeader: appVersionCode mismatch takes priority over later mismatches
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadFileHeader_AppVersionMismatchPriority)
{
    auto writeHeader = ModulesSnapshotHelper::SnapshotVersionInfo::New(10, "5.0.1", "desc-A");
    ASSERT_NE(writeHeader, nullptr);
    auto readHeader = ModulesSnapshotHelper::SnapshotVersionInfo::New(20, "5.0.2", "desc-B");
    ASSERT_NE(readHeader, nullptr);

    size_t bufferSize = writeHeader->Size() + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "ReadFileHeader_WriteAllMismatch");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, writeHeader);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "ReadFileHeader_AppVersionMismatchPriority");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, readHeader);
    EXPECT_FALSE(result.empty());
    // appVersionCode mismatch should be reported first (implementation checks it before ROM/desc)
    EXPECT_NE(result.find("application version code"), std::string::npos);
}

// ReadFileHeader: buffer exactly sized to header->Size()
HWTEST_F_L0(ModulesSnapshotHelperTest, ReadFileHeader_BufferExactlySized)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(5, "RomVer", "DescInfo");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size();
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap memMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(memMap, "ReadFileHeader_BufferExactlySized_Write");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "ReadFileHeader_BufferExactlySized_Read");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, header);
    EXPECT_TRUE(result.empty());
}

// WriteFileHeader + ReadFileHeader roundtrip with empty strings
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteAndReadFileHeader_RoundtripEmptyStrings)
{
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(0, "", "");
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size() + 64;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap writeMemMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(writeMemMap, "RoundtripEmpty_Write");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "RoundtripEmpty_Read");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, header);
    EXPECT_TRUE(result.empty());
}

// WriteFileHeader + ReadFileHeader roundtrip with long strings
HWTEST_F_L0(ModulesSnapshotHelperTest, WriteAndReadFileHeader_RoundtripLongStrings)
{
    std::string longRomVersion = "OpenHarmony-5.0.1.100-20250603-Long-Release-Build-Version-String";
    std::string longDescription = "This is a very long snapshot file description that contains multiple words "
                                  "and phrases to test whether the serialization and deserialization "
                                  "of the SnapshotVersionInfo handles variable-length strings correctly "
                                  "without truncation or buffer overflow issues.";
    auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(99999, longRomVersion, longDescription);
    ASSERT_NE(header, nullptr);

    size_t bufferSize = header->Size() + 128;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemMap writeMemMap(buffer.data(), bufferSize);
    FileMemMapWriter writer(writeMemMap, "RoundtripLong_Write");

    bool writeResult = ModulesSnapshotHelper::WriteFileHeader(writer, header);
    ASSERT_TRUE(writeResult);

    MemMap readMemMap(buffer.data(), bufferSize);
    FileMemMapReader reader(readMemMap, []() {}, "RoundtripLong_Read");

    std::string result = ModulesSnapshotHelper::ReadFileHeader(reader, header);
    EXPECT_TRUE(result.empty());
}

} // namespace panda::test