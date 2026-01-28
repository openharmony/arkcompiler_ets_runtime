/**
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

#include <cstdint>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#include "aot_args_verify.h"
#include "aot_compiler_constants.h"

using namespace testing::ext;

namespace OHOS::ArkCompiler {

class AotArgsVerifyTest : public testing::Test {
public:
    AotArgsVerifyTest() {}
    virtual ~AotArgsVerifyTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: AotArgsVerifyTest_001
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with invalid JSON
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_001, TestSize.Level0)
{
    std::string pkgInfoStr = "{invalid_json}";
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_002
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with missing pgoDir
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_002, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"bundleName\": \"test_app\"}"; // Missing pgoDir
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_003
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with missing bundleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_003, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"pgoDir\": \"/data/local/ark-profile/100/test_app\"}"; // Missing bundleName
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_004
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with non-string pgoDir
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_004, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"pgoDir\": 123, \"bundleName\": \"test_app\"}"; // Non-string pgoDir
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_005
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with non-string bundleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_005, TestSize.Level0)
{
    std::string pkgInfoStr =
        "{\"pgoDir\": \"/data/local/ark-profile/100/test_app\", \"bundleName\": 123}"; // Non-string bundleName
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_006
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with empty JSON array
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_006, TestSize.Level0)
{
    std::string pkgInfoStr = "[]";  // 空数组会使 pkgInfoJson.empty() 返回 true
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_007
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with empty pgoDir value
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_007, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"pgoDir\": \"\", \"bundleName\": \"test_app\"}";
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_008
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with empty bundleName value
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_008, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
                              "\"bundleName\": \"\"}";
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseBundleNameFromPkgInfo_001
 * @tc.desc: Test AotArgsVerify::ParseBundleNameFromPkgInfo with valid JSON
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseBundleNameFromPkgInfo_001, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"bundleName\": \"test_app\"}";
    std::string bundleName;

    bool result = AotArgsVerify::ParseBundleNameFromPkgInfo(pkgInfoStr, bundleName);

    EXPECT_TRUE(result);
    EXPECT_EQ(bundleName, "test_app");
}

/**
 * @tc.name: AotArgsVerifyTest_ParseBundleNameFromPkgInfo_002
 * @tc.desc: Test AotArgsVerify::ParseBundleNameFromPkgInfo with invalid JSON
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseBundleNameFromPkgInfo_002, TestSize.Level0)
{
    std::string pkgInfoStr = "{invalid_json}";
    std::string bundleName;

    bool result = AotArgsVerify::ParseBundleNameFromPkgInfo(pkgInfoStr, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseBundleNameFromPkgInfo_003
 * @tc.desc: Test AotArgsVerify::ParseBundleNameFromPkgInfo with missing bundleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseBundleNameFromPkgInfo_003, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\"}";
    std::string bundleName;

    bool result = AotArgsVerify::ParseBundleNameFromPkgInfo(pkgInfoStr, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseBundleNameFromPkgInfo_004
 * @tc.desc: Test AotArgsVerify::ParseBundleNameFromPkgInfo with non-string bundleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseBundleNameFromPkgInfo_004, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"bundleName\": 123}";
    std::string bundleName;

    bool result = AotArgsVerify::ParseBundleNameFromPkgInfo(pkgInfoStr, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseBundleNameFromPkgInfo_005
 * @tc.desc: Test AotArgsVerify::ParseBundleNameFromPkgInfo with empty JSON
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseBundleNameFromPkgInfo_005, TestSize.Level0)
{
    std::string pkgInfoStr = "{}";
    std::string bundleName;

    bool result = AotArgsVerify::ParseBundleNameFromPkgInfo(pkgInfoStr, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseBundleNameFromPkgInfo_006
 * @tc.desc: Test AotArgsVerify::ParseBundleNameFromPkgInfo with empty bundleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseBundleNameFromPkgInfo_006, TestSize.Level0)
{
    std::string pkgInfoStr = "{\"bundleName\": \"\"}";
    std::string bundleName;

    bool result = AotArgsVerify::ParseBundleNameFromPkgInfo(pkgInfoStr, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseBundleNameFromPkgInfo_007
 * @tc.desc: Test AotArgsVerify::ParseBundleNameFromPkgInfo with empty string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseBundleNameFromPkgInfo_007, TestSize.Level0)
{
    std::string pkgInfoStr = "";
    std::string bundleName;

    bool result = AotArgsVerify::ParseBundleNameFromPkgInfo(pkgInfoStr, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_009
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with valid path (no traversal)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_009, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_010
 * @tc.desc: Test AotArgsVerify::ValidateArkCacheFilePaths with valid paths
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_010, TestSize.Level0)
{
    // Create the required subdirectory
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create dummy files for testing
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    anFile.close();

    bool result = AotArgsVerify::ValidateArkCacheFilePaths(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test",
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an",
        "test_app"
    );

    EXPECT_TRUE(result);

    // Clean up created files and directories
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test_app");
}

/**
 * @tc.name: AotArgsVerifyTest_011
 * @tc.desc: Test AotArgsVerify::ValidateArkProfilePath with valid path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_011, TestSize.Level0)
{
    // Create the required subdirectory
    const std::string testAppDir = "/data/app/el1/100/aot_compiler/ark_profile/test_app";
    mkdir(testAppDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::ValidateArkProfilePath(testAppDir, "test_app");

    EXPECT_TRUE(result);

    // Clean up created directory
    rmdir(testAppDir.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_012
 * @tc.desc: Test AotArgsVerify::ValidateArkProfilePath with different user ID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_012, TestSize.Level0)
{
    // Create the required subdirectory
    const std::string myBundleDir = "/data/app/el1/100/aot_compiler/ark_profile/my_bundle";
    mkdir(myBundleDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::ValidateArkProfilePath(myBundleDir, "my_bundle");

    EXPECT_TRUE(result);

    // Clean up created directory
    rmdir(myBundleDir.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_013
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with valid JSON
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_013, TestSize.Level0)
{
    std::string pkgInfoStr =
        "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/com.valid.test\", "
        "\"bundleName\": \"com.valid.test\"}";
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_TRUE(result);
    EXPECT_EQ(pgoDir, "/data/app/el1/100/aot_compiler/ark_profile/com.valid.test");
    EXPECT_EQ(bundleName, "com.valid.test");
}

/**
 * @tc.name: AotArgsVerifyTest_014
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_014, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("/data/local/../etc"); // Path traversal

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_015
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with Windows-style path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_015, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("\\data\\local\\..\\etc"); // Windows-style path traversal

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_016
 * @tc.desc: Test AotArgsVerify::ValidateAndResolvePath with Windows-style path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_016, TestSize.Level0)
{
    std::string outputPath;
    bool result = AotArgsVerify::ValidateAndResolvePath("/data/app/el1/100\\..\\test", outputPath);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_017
 * @tc.desc: Test AotArgsVerify::ValidateArkProfilePath with path not matching pattern
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_017, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidateArkProfilePath("/data/app/el1/100/wrong_path/com.test.app", "com.test.app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_018
 * @tc.desc: Test AotArgsVerify::ValidateArkProfilePath with bundleName not in path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_018, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidateArkProfilePath(
        "/data/app/el1/100/aot_compiler/ark_profile/com.other.app", "com.test.app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_019
 * @tc.desc: Test AotArgsVerify::ParsePgoDirAndBundleName with discarded JSON
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_019, TestSize.Level0)
{
    std::string pkgInfoStr = "";
    std::string pgoDir, bundleName;

    bool result = AotArgsVerify::ParsePgoDirAndBundleName(pkgInfoStr, pgoDir, bundleName);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_020
 * @tc.desc: Test AotArgsVerify::ValidateArkCacheFilePaths with invalid path format
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_020, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidateArkCacheFilePaths(
        "invalid_format_file", "another_invalid_format", "test_bundle");

    // Should fail because paths don't have directory separators
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_021
 * @tc.desc: Test AotArgsVerify::ValidateArkCacheFilePaths with path traversal in both files
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_021, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidateArkCacheFilePaths(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/../etc/test",
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/../etc/test.an",
        "test_app"
    );

    // Should fail because of path traversal in both file paths
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_022
 * @tc.desc: Test AotArgsVerify::ValidateArkCacheFilePaths with files in different directories
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_022, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidateArkCacheFilePaths(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app1/test",
        "/data/app/el1/public/aot_compiler/ark_cache/test_app2/test.an",
        "test_app"
    );

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_023
 * @tc.desc: Test AotArgsVerify::ValidateArkCacheFilePaths with non-existent parent directory
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_023, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidateArkCacheFilePaths(
        "/data/app/el1/public/nonexistent/test_app/test",
        "/data/app/el1/public/nonexistent/test_app/test.an",
        "test_app"
    );

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_024
 * @tc.desc: Test AotArgsVerify::CheckAndGetAotAndAnFiles with valid inputs
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_024, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    std::string aotFile;
    std::string anFile;
    bool result = AotArgsVerify::CheckAndGetAotAndAnFiles(argsMap, aotFile, anFile);

    EXPECT_TRUE(result);
    EXPECT_EQ(aotFile, "/data/app/el1/public/aot_compiler/ark_cache/test_app/test");
    EXPECT_EQ(anFile, "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
}

/**
 * @tc.name: AotArgsVerifyTest_025
 * @tc.desc: Test AotArgsVerify::CheckAndGetAotAndAnFiles with missing AOT_FILE
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_025, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    // AOT_FILE is intentionally not added

    std::string aotFile;
    std::string anFile;
    bool result = AotArgsVerify::CheckAndGetAotAndAnFiles(argsMap, aotFile, anFile);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_026
 * @tc.desc: Test AotArgsVerify::CheckAndGetAotAndAnFiles with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_026, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    // AN_FILE_NAME is intentionally not added

    std::string aotFile;
    std::string anFile;
    bool result = AotArgsVerify::CheckAndGetAotAndAnFiles(argsMap, aotFile, anFile);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_027
 * @tc.desc: Test AotArgsVerify::CheckAndGetAotAndAnFiles with both files missing
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_027, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    // Both AOT_FILE and AN_FILE_NAME are intentionally not added

    std::string aotFile;
    std::string anFile;
    bool result = AotArgsVerify::CheckAndGetAotAndAnFiles(argsMap, aotFile, anFile);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_028
 * @tc.desc: Test AotArgsVerify::CheckAndGetAotAndAnFiles with empty argsMap
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_028, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;

    std::string aotFile;
    std::string anFile;
    bool result = AotArgsVerify::CheckAndGetAotAndAnFiles(argsMap, aotFile, anFile);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_001
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with valid inputs
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_001, TestSize.Level0)
{
    // Create required directories
    mkdir("/data/app/el1/100/aot_compiler/ark_profile/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create dummy .an file
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    anFile.close();

    std::unordered_map<std::string, std::string> argsMap;
    std::string pkgInfoStr = "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
                             "\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfoStr;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);
    EXPECT_TRUE(result);

    // Clean up
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test_app");
    rmdir("/data/app/el1/100/aot_compiler/ark_profile/test_app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_002
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with missing COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    // COMPILER_PKG_INFO is intentionally not added

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_001
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with valid inputs
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_001, TestSize.Level0)
{
    // Create required directory
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create dummy .an file
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    anFile.close();

    std::unordered_map<std::string, std::string> argsMap;
    std::string pkgInfoStr = "{\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfoStr;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_TRUE(result);

    // Clean up
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test_app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_002
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with missing COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    // COMPILER_PKG_INFO is intentionally not added

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkStaticAotArgs_001
 * @tc.desc: Test AotArgsVerify::CheckFrameworkStaticAotArgs with valid inputs
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkStaticAotArgs_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/service/el1/public/for-all-app/framework_ark_cache/test.abc.an";

    bool result = AotArgsVerify::CheckFrameworkStaticAotArgs(argsMap);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkStaticAotArgs_002
 * @tc.desc: Test AotArgsVerify::CheckFrameworkStaticAotArgs with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkStaticAotArgs_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    // AN_FILE_NAME is intentionally not added

    bool result = AotArgsVerify::CheckFrameworkStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkStaticAotArgs_003
 * @tc.desc: Test AotArgsVerify::CheckFrameworkStaticAotArgs with path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkStaticAotArgs_003, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/service/el1/public/for-all-app/framework_ark_cache/../test.abc.an";

    bool result = AotArgsVerify::CheckFrameworkStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkStaticAotArgs_004
 * @tc.desc: Test AotArgsVerify::CheckFrameworkStaticAotArgs with wrong path prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkStaticAotArgs_004, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test.abc.an";

    bool result = AotArgsVerify::CheckFrameworkStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_003
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with invalid JSON in COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_003, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{invalid_json}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_004
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with missing AOT_FILE
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_004, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    const std::string pkgInfo = "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
                                  "\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfo;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    // AOT_FILE is intentionally not added

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_005
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_005, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    const std::string pkgInfo = "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
                                  "\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfo;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    // AN_FILE_NAME is intentionally not added

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_006
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with path traversal in pgoDir
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_006, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    const std::string pkgInfo = "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/../etc\", "
                                  "\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfo;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_007
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with ark cache files in wrong directory
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_007, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    const std::string pkgInfo = "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
                                  "\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfo;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/wrong_location/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/wrong_location/test_app/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_003
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with invalid JSON in COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_003, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{invalid_json}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_004
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with empty JSON in COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_004, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_005
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with missing AOT_FILE
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_005, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    // AOT_FILE is intentionally not added

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_006
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_006, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    // AN_FILE_NAME is intentionally not added

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_007
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with empty bundleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_007, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_008
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with wrong ark cache directory
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_008, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"test_app\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/wrong_location/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/wrong_location/test_app/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ValidateAndResolvePath_001
 * @tc.desc: Test AotArgsVerify::ValidateAndResolvePath with non-existent path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidateAndResolvePath_001, TestSize.Level0)
{
    std::string outputPath;
    // Use a non-existent path to test RealPath failure
    bool result = AotArgsVerify::ValidateAndResolvePath("/nonexistent/path/that/does/not/exist", outputPath);

    // Should fail because RealPath will fail
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFileName_001
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFileName with valid app arkcache path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFileName_001, TestSize.Level0)
{
    // Create the required directory structure
    mkdir("/data/app/el1/public/aot_compiler/ark_cache", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.example.app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create a test file
    std::ofstream testFile("/data/app/el1/public/aot_compiler/ark_cache/com.example.app/test.abc");
    testFile.close();

    bool result = AotArgsVerify::CheckCodeSignArkCacheFileName(
        "/data/app/el1/public/aot_compiler/ark_cache/com.example.app/test.abc");

    EXPECT_TRUE(result);

    // Clean up
    unlink("/data/app/el1/public/aot_compiler/ark_cache/com.example.app/test.abc");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.example.app");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFileName_002
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFileName with valid framework arkcache path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFileName_002, TestSize.Level0)
{
    // Create the required directory structure
    mkdir("/data/service/el1/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/service/el1/public/for-all-app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/service/el1/public/for-all-app/framework_ark_cache", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create a test file
    std::ofstream testFile("/data/service/el1/public/for-all-app/framework_ark_cache/test.abc");
    testFile.close();

    bool result = AotArgsVerify::CheckCodeSignArkCacheFileName(
        "/data/service/el1/public/for-all-app/framework_ark_cache/test.abc");

    EXPECT_TRUE(result);

    // Clean up
    unlink("/data/service/el1/public/for-all-app/framework_ark_cache/test.abc");
    rmdir("/data/service/el1/public/for-all-app/framework_ark_cache");
    rmdir("/data/service/el1/public/for-all-app");
    rmdir("/data/service/el1/public");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFileName_003
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFileName with path traversal attack
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFileName_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFileName(
        "/data/app/el1/public/aot_compiler/ark_cache/../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFileName_004
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFileName with invalid directory prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFileName_004, TestSize.Level0)
{
    // Create the required directory structure
    mkdir("/data/local", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/local/tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create a test file
    std::ofstream testFile("/data/local/tmp/test.abc");
    testFile.close();

    bool result = AotArgsVerify::CheckCodeSignArkCacheFileName("/data/local/tmp/test.abc");

    EXPECT_FALSE(result);

    // Clean up
    unlink("/data/local/tmp/test.abc");
    rmdir("/data/local/tmp");
    rmdir("/data/local");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFileName_005
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFileName with empty path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFileName_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFileName("");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFileName_006
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFileName with non-existent path (RealPath failure)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFileName_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFileName(
        "/data/app/el1/public/aot_compiler/ark_cache/nonexistent/file.abc");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ValidatePathOnlyTraverse_001
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with standalone ".."
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidatePathOnlyTraverse_001, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("..");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ValidatePathOnlyTraverse_002
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with multiple ".." traversals
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidatePathOnlyTraverse_002, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("/data/../../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ValidatePathOnlyTraverse_003
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with ".." in filename (not traversal)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidatePathOnlyTraverse_003, TestSize.Level0)
{
    // Note: Current implementation rejects this (conservative approach)
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("/data/app..name/file");

    EXPECT_FALSE(result);  // Implementation rejects ".." anywhere in path
}

/**
 * @tc.name: AotArgsVerifyTest_ValidatePathOnlyTraverse_004
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with "../" at beginning
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidatePathOnlyTraverse_004, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ValidatePathOnlyTraverse_005
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with ".." at end
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidatePathOnlyTraverse_005, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("/data/app/..");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ValidatePathOnlyTraverse_006
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with Windows-style ".." at beginning
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidatePathOnlyTraverse_006, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("..\\etc\\passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ValidatePathOnlyTraverse_007
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with single dot "." (current directory)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidatePathOnlyTraverse_007, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("/data/./file");

    EXPECT_TRUE(result);  // Single dot is not a traversal
}

/**
 * @tc.name: AotArgsVerifyTest_ValidatePathOnlyTraverse_008
 * @tc.desc: Test AotArgsVerify::ValidatePathOnlyTraverse with empty path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ValidatePathOnlyTraverse_008, TestSize.Level0)
{
    bool result = AotArgsVerify::ValidatePathOnlyTraverse("");

    EXPECT_TRUE(result);  // Empty path doesn't contain traversal
}

} // namespace OHOS::ArkCompiler
