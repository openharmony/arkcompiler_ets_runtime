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
#include "jsnapi_expo.h"
#include <string>
#include <string_view>
#include <fstream>

#define private public
#define protected public
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#undef private
#undef protected

#include "gtest/gtest.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <system_error>

namespace panda::test {
using namespace panda::ecmascript;

class ModulesSnapshotHelperTest : public BaseTestWithOutScope {
public:
    constexpr static const std::string_view MODULES_SNAPSHOT_FILE_DIR = "./";
    constexpr static const std::string_view EXISTING_TEST_FILE_NAME = "./test_module_snapshot_helper.txt";
    constexpr static const std::string_view NOT_EXISTING_TEST_FILE_NAME = "./non_existing_test_file.txt";

    void SetUp() override {}
    void TearDown() override {}
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

} // namespace panda::test