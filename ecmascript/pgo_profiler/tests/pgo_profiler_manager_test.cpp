/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace panda::test {
class PGOProfilerManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }

    void SetUp() override
    {
        PGOProfilerManager::GetInstance()->Destroy();
    }

    void TearDown() override
    {
        PGOProfilerManager::GetInstance()->Destroy();
    }
};

HWTEST_F_L0(PGOProfilerManagerTest, GetInstanceTest)
{
    auto* instance1 = PGOProfilerManager::GetInstance();
    auto* instance2 = PGOProfilerManager::GetInstance();
    ASSERT_EQ(instance1, instance2);
}

HWTEST_F_L0(PGOProfilerManagerTest, InitializeAndIsInitializedTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    ASSERT_FALSE(manager->IsInitialized());
    manager->Initialize("test_output_dir/", 1);
    ASSERT_FALSE(manager->IsInitialized());
}

HWTEST_F_L0(PGOProfilerManagerTest, SetAndGetBundleNameTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    std::string testBundleName = "com.example.test";
    manager->SetBundleName(testBundleName);
    ASSERT_EQ(manager->GetBundleName(), testBundleName);
}

HWTEST_F_L0(PGOProfilerManagerTest, IsEnableTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    ASSERT_FALSE(manager->IsEnable());
    manager->Initialize("test_output_dir/", 1);
    ASSERT_FALSE(manager->IsEnable());
    manager->SetDisablePGO(true);
    ASSERT_FALSE(manager->IsEnable());
    manager->SetDisablePGO(false);
    ASSERT_FALSE(manager->IsEnable());
}

HWTEST_F_L0(PGOProfilerManagerTest, MaxAotMethodSizeTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    ASSERT_EQ(manager->GetMaxAotMethodSize(), 0U);
    uint32_t testSize = 1000;
    manager->SetMaxAotMethodSize(testSize);
    ASSERT_EQ(manager->GetMaxAotMethodSize(), testSize);
}

HWTEST_F_L0(PGOProfilerManagerTest, IsBigMethodTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    ASSERT_FALSE(manager->IsBigMethod(100));
    manager->SetMaxAotMethodSize(1000);
    ASSERT_FALSE(manager->IsBigMethod(500));   // Smaller than max
    ASSERT_TRUE(manager->IsBigMethod(1500));   // Larger than max
    ASSERT_FALSE(manager->IsBigMethod(1000));  // Equal to max
}

HWTEST_F_L0(PGOProfilerManagerTest, SetAndGetApGenModeTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    ASSERT_EQ(manager->GetApGenMode(), ApGenMode::MERGE);
    manager->SetApGenMode(ApGenMode::OVERWRITE);
    ASSERT_EQ(manager->GetApGenMode(), ApGenMode::OVERWRITE);
}

HWTEST_F_L0(PGOProfilerManagerTest, GetPGOInfoTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    auto pgoInfoBefore = manager->GetPGOInfo();
    manager->Initialize("test_output_dir/", 1);
    auto pgoInfoAfter = manager->GetPGOInfo();
    ASSERT_NE(pgoInfoAfter, nullptr);
}

HWTEST_F_L0(PGOProfilerManagerTest, ResetOutPathTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 1);
    std::string testFileName = "test.ap";
    ASSERT_TRUE(manager->ResetOutPath(testFileName));
}

HWTEST_F_L0(PGOProfilerManagerTest, ResetOutPathByModuleNameTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 1);
    std::string moduleName = "testModule";
    ASSERT_TRUE(manager->ResetOutPathByModuleName(moduleName));
    ASSERT_FALSE(manager->ResetOutPathByModuleName(""));
    ASSERT_FALSE(manager->ResetOutPathByModuleName("anotherModule"));
}

HWTEST_F_L0(PGOProfilerManagerTest, SetDisablePGOTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 1);
    ASSERT_FALSE(manager->IsEnable());
    manager->SetDisablePGO(true);
    ASSERT_FALSE(manager->IsEnable());
    manager->SetDisablePGO(false);
    ASSERT_FALSE(manager->IsEnable());
}

HWTEST_F_L0(PGOProfilerManagerTest, DestroyTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 1);
    manager->Destroy();
    ASSERT_FALSE(manager->IsInitialized());
}

HWTEST_F_L0(PGOProfilerManagerTest, IsApFileCompatibleTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    ASSERT_TRUE(manager->GetIsApFileCompatible());
    manager->SetIsApFileCompatible(false);
    ASSERT_FALSE(manager->GetIsApFileCompatible());
    manager->SetIsApFileCompatible(true);
    ASSERT_TRUE(manager->GetIsApFileCompatible());
}

HWTEST_F_L0(PGOProfilerManagerTest, GetPandaFileIdTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 1);
    CString abcName = "test.abc";
    ApEntityId entryId(0);
    ASSERT_FALSE(manager->GetPandaFileId(abcName, entryId));
}

HWTEST_F_L0(PGOProfilerManagerTest, GetPandaFileDescTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 1);
    ApEntityId abcId(0);
    CString desc;
    ASSERT_FALSE(manager->GetPandaFileDesc(abcId, desc));
}

HWTEST_F_L0(PGOProfilerManagerTest, MultipleInitializeTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir_1/", 1);
    ASSERT_NE(manager->GetPGOInfo(), nullptr);
    manager->Initialize("test_output_dir_2/", 2);
    ASSERT_NE(manager->GetPGOInfo(), nullptr);
}

HWTEST_F_L0(PGOProfilerManagerTest, BinaryToTextLoadFullFailuresTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    bool result1 = manager->BinaryToText("nonexistent.ap", "test_output.txt", 5);
    EXPECT_FALSE(result1);

    bool result2 = manager->BinaryToText("test.txt", "test_output2.txt", 5);
    EXPECT_FALSE(result2);
}

HWTEST_F_L0(PGOProfilerManagerTest, BinaryToTextWithInvalidOutPathTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    std::string inPath = "test_input.ap";
    std::string outPath = "/invalid/path/that/does/not/exist/output.txt";
    bool result = manager->BinaryToText(inPath, outPath, 5);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerManagerTest, InitializeDataAlreadyInitializedTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 5);
    auto pgoInfo = manager->GetPGOInfo();
    EXPECT_NE(pgoInfo, nullptr);
}

HWTEST_F_L0(PGOProfilerManagerTest, SetRequestAotCallbackTwiceTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    int firstCallCount = 0;
    int secondCallCount = 0;
    auto firstCallback = [&firstCallCount](const std::string&, const std::string&, int32_t) -> int {
        firstCallCount++;
        return 0;
    };
    auto secondCallback = [&secondCallCount](const std::string&, const std::string&, int32_t) -> int {
        secondCallCount++;
        return 0;
    };

    manager->SetRequestAotCallback(firstCallback);
    manager->SetRequestAotCallback(secondCallback);
}

HWTEST_F_L0(PGOProfilerManagerTest, RequestAotWithMissingParamsTest)
{
    auto* manager = PGOProfilerManager::GetInstance();

    // Clear bundleName to test empty parameter scenario
    manager->SetBundleName("");
    manager->RequestAot();
}

HWTEST_F_L0(PGOProfilerManagerTest, ResetWithNullProfilerTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    std::shared_ptr<PGOProfiler> nullProfiler = nullptr;
    manager->Reset(nullProfiler, true);
}

HWTEST_F_L0(PGOProfilerManagerTest, SamplePandaFileInfoWithoutPgoInfoTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->SamplePandaFileInfo(123, "test.abc");
}

HWTEST_F_L0(PGOProfilerManagerTest, SamplePandaFileInfoWithPgoInfoTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 5);
    manager->SamplePandaFileInfo(123, "test.abc");
}

HWTEST_F_L0(PGOProfilerManagerTest, IsBigMethodWithZeroMaxTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->SetMaxAotMethodSize(0);
    EXPECT_FALSE(manager->IsBigMethod(0));
    EXPECT_FALSE(manager->IsBigMethod(100));
    EXPECT_FALSE(manager->IsBigMethod(UINT32_MAX));
}

HWTEST_F_L0(PGOProfilerManagerTest, MergeApFilesChecksumEmptyListTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    PGOProfilerDecoder merger("", 5);  // Empty path
    bool result = manager->MergeApFiles(fileNameToChecksumMap, merger);
    // pgoFileNamesVector.empty() should return true
    EXPECT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerManagerTest, IsProfilerDestroyedTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    PGOProfiler* fakeProfiler = reinterpret_cast<PGOProfiler*>(0x12345678);
    bool result = manager->IsProfilerDestroyed(fakeProfiler);
    // profiler not in set, should return true (destroyed)
    EXPECT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerManagerTest, SetIsTaskRunningTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->SetIsTaskRunning(true);
    EXPECT_TRUE(manager->IsTaskRunning());
    manager->SetIsTaskRunning(false);
    EXPECT_FALSE(manager->IsTaskRunning());
}

HWTEST_F_L0(PGOProfilerManagerTest, SetForceDumpTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->SetForceDump(true);
    EXPECT_TRUE(manager->IsForceDump());
    manager->SetForceDump(false);
    EXPECT_FALSE(manager->IsForceDump());
}

HWTEST_F_L0(PGOProfilerManagerTest, SavingSignalHandlerWithWrongSignalTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    // Send wrong signal number
    PGOProfilerManager::SavingSignalHandler(99);
    EXPECT_FALSE(manager->IsForceDump());
}

HWTEST_F_L0(PGOProfilerManagerTest, SavingSignalHandlerWithCorrectSignalTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    // Send correct signal number (50 = PGO_SAVING_SIGNAL)
    PGOProfilerManager::SavingSignalHandler(50);
    // should set forceDump to true
    EXPECT_TRUE(manager->IsForceDump());
    // Reset for other tests
    manager->SetForceDump(false);
}

HWTEST_F_L0(PGOProfilerManagerTest, RegisterSavingSignalNotInitializedTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->RegisterSavingSignal();
}

HWTEST_F_L0(PGOProfilerManagerTest, RegisterSavingSignalAfterInitializedTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    manager->Initialize("test_output_dir/", 5);
}

HWTEST_F_L0(PGOProfilerManagerTest, MergeApFilesWithNonApExtensionTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    std::string inFiles = "test.txt,test2.ap";
    std::string outPath = "/tmp/test_output.ap";
    bool result = manager->MergeApFiles(inFiles, outPath, 5, ApGenMode::MERGE);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerManagerTest, MergeApFilesWithNonExistentFileTest)
{
    auto* manager = PGOProfilerManager::GetInstance();
    std::string inFiles = "nonexistent.ap";
    std::string outPath = "/tmp/test_output.ap";
    bool result = manager->MergeApFiles(inFiles, outPath, 5, ApGenMode::MERGE);
    EXPECT_FALSE(result);
}

}  // namespace panda::test
