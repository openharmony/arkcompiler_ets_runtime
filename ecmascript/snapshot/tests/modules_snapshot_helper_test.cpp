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
#include "jsnapi_expo.h"

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

} // namespace panda::test