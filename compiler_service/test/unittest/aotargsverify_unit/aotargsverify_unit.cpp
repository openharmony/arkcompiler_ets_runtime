/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http:
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
 * @tc.name: AotArgsVerifyTest_009
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with valid path (no traversal)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_009, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_010
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with valid paths
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_010, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    anFile.close();

    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_TRUE(result);
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test_app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_002
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with AOT_FILE missing
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_003
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with AN_FILE_NAME missing
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_003, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_004
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with both AOT_FILE and AN_FILE_NAME missing
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_004, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_005
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with mismatched file suffix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_005, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.wrong";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_006
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with path traversal in aotFile
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_006, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/../etc/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/../etc/test.an";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_007
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with invalid cache directory path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_007, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/local/tmp/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/local/tmp/test.an";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_011
 * @tc.desc: Test AotArgsVerify::CheckArkProfilePath with valid path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_011, TestSize.Level0)
{
    const std::string testAppDir = "/data/app/el1/100/aot_compiler/ark_profile/test_app";
    mkdir(testAppDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::CheckArkProfilePath(testAppDir, "test_app");

    EXPECT_TRUE(result);
    rmdir(testAppDir.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_012
 * @tc.desc: Test AotArgsVerify::CheckArkProfilePath with different user ID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_012, TestSize.Level0)
{
    const std::string myBundleDir = "/data/app/el1/100/aot_compiler/ark_profile/my_bundle";
    mkdir(myBundleDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::CheckArkProfilePath(myBundleDir, "my_bundle");

    EXPECT_TRUE(result);
    rmdir(myBundleDir.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_013
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_013, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/local/../etc");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_015
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with Windows-style path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_015, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("\\data\\local\\..\\etc");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_017
 * @tc.desc: Test AotArgsVerify::CheckArkProfilePath with path not matching pattern
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_017, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkProfilePath("/data/app/el1/100/wrong_path/com.test.app", "com.test.app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_018
 * @tc.desc: Test AotArgsVerify::CheckArkProfilePath with bundleName not in path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_018, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkProfilePath(
        "/data/app/el1/100/aot_compiler/ark_profile/com.other.app", "com.test.app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_019
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with invalid path format
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_019, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "invalid_format_file";
    argsMap[ArgsIdx::AN_FILE_NAME] = "another_invalid_format";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_bundle");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_021
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with path traversal in both files
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_021, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/../etc/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/../etc/test.an";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_022
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with files in different directories
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_022, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app1/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app2/test.an";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_023
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with non-existent parent directory
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_023, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/nonexistent/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/nonexistent/test_app/test.an";

    bool result = AotArgsVerify::CheckArkCacheFiles(argsMap, "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_001
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with valid inputs
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_001, TestSize.Level0)
{
#if defined(PANDA_TARGET_OHOS)
    GTEST_SKIP() << "Test skipped on OHOS - requires valid HAP files";
#else
    mkdir("/data/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/100/aot_compiler/ark_profile/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test/test.hap");
    hapFile << "HAP";
    hapFile.close();
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    anFile.close();

    std::unordered_map<std::string, std::string> argsMap;
    std::string pkgInfoStr = "{\"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
                             "\"bundleName\": \"test_app\", "
                             "\"pkgPath\": \"/data/test/test.hap\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfoStr;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);
    EXPECT_TRUE(result);
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test_app");
    rmdir("/data/app/el1/100/aot_compiler/ark_profile/test_app");
    unlink("/data/test/test.hap");
    rmdir("/data/test");
#endif
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

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
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
    const std::string pkgInfo = "{\"bundleName\": \"test_app\", \"pkgPath\": \"/data/test/test.hap\", "
        "\"abcName\": \"ets/modules.abc\", \"moduleName\": \"module\", "
        "\"appIdentifier\": \"sig\", \"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
        "\"abcOffset\": \"0\", \"abcSize\": \"64\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfo;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

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
    const std::string pkgInfo = "{\"bundleName\": \"test_app\", \"pkgPath\": \"/data/test/test.hap\", "
        "\"abcName\": \"ets/modules.abc\", \"moduleName\": \"module\", "
        "\"appIdentifier\": \"sig\", \"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
        "\"abcOffset\": \"0\", \"abcSize\": \"64\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfo;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";

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
    const std::string pkgInfo = "{\"bundleName\": \"test_app\", \"pkgPath\": \"/data/test/test.hap\", "
        "\"abcName\": \"ets/modules.abc\", \"moduleName\": \"module\", "
        "\"appIdentifier\": \"sig\", \"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/../etc\", "
        "\"abcOffset\": \"0\", \"abcSize\": \"64\"}";
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
    const std::string pkgInfo = "{\"bundleName\": \"test_app\", \"pkgPath\": \"/data/test/test.hap\", "
        "\"abcName\": \"ets/modules.abc\", \"moduleName\": \"module\", "
        "\"appIdentifier\": \"sig\", \"pgoDir\": \"/data/app/el1/100/aot_compiler/ark_profile/test_app\", "
        "\"abcOffset\": \"0\", \"abcSize\": \"64\"}";
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = pkgInfo;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/wrong_location/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/wrong_location/test_app/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_PartialMode_001
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with partial mode and valid pgoDir
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_PartialMode_001, TestSize.Level0)
{
#if defined(PANDA_TARGET_OHOS)
    GTEST_SKIP() << "Test skipped on OHOS - requires valid HAP files";
#else
    mkdir("/data/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/100/aot_compiler/ark_profile/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream hapFile("/data/test/test.hap");
    hapFile << "HAP";
    hapFile.close();
    std::ofstream("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an").close();

    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"test_app\",\"pkgPath\":\"/data/test/test.hap\","
         "\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/test_app\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\"}";
    argsMap[ArgsIdx::TARGET_COMPILER_MODE] = "partial";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_TRUE(result);

    unlink("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test_app");
    rmdir("/data/app/el1/100/aot_compiler/ark_profile/test_app");
    unlink("/data/test/test.hap");
    rmdir("/data/test");
#endif
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_PartialMode_002
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with partial mode and invalid pgoDir path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_PartialMode_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"test_app\",\"pkgPath\":\"/data/test/test.hap\","
         "\"pgoDir\":\"/data/app/el2/100/aot_compiler/ark_profile/test_app\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\"}";
    argsMap[ArgsIdx::TARGET_COMPILER_MODE] = "partial";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_PartialMode_003
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with non-partial mode ignores pgoDir
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_PartialMode_003, TestSize.Level0)
{
#if defined(PANDA_TARGET_OHOS)
    GTEST_SKIP() << "Test skipped on OHOS - requires valid HAP files";
#else
    mkdir("/data/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream hapFile("/data/test/test.hap");
    hapFile << "HAP";
    hapFile.close();
    std::ofstream("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an").close();

    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"test_app\",\"pkgPath\":\"/data/test/test.hap\","
         "\"pgoDir\":\"/data/app/el2/100/aot_compiler/ark_profile/test_app\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\"}";
    argsMap[ArgsIdx::TARGET_COMPILER_MODE] = "full";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_TRUE(result);

    unlink("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test_app");
    unlink("/data/test/test.hap");
    rmdir("/data/test");
#endif
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
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\": \"test_app\", \"pkgPath\": \"/data/test/test.hap\", "
         "\"abcName\": \"ets/modules_static.abc\", \"moduleName\": \"module\", "
         "\"appIdentifier\": \"sig\", \"pgoDir\": \"/data/pgo\", "
         "\"abcOffset\": \"0\", \"abcSize\": \"64\"}";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";

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
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\": \"test_app\", \"pkgPath\": \"/data/test/test.hap\", "
         "\"abcName\": \"ets/modules_static.abc\", \"moduleName\": \"module\", "
         "\"appIdentifier\": \"sig\", \"pgoDir\": \"/data/pgo\", "
         "\"abcOffset\": \"0\", \"abcSize\": \"64\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_001
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with valid app arkcache path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_001, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.example.app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(
        "/data/app/el1/public/aot_compiler/ark_cache/com.example.app");

    EXPECT_TRUE(result);
    unlink("/data/app/el1/public/aot_compiler/ark_cache/com.example.app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_003
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with path traversal attack
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(
        "/data/app/el1/public/aot_compiler/ark_cache/../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_004
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with invalid directory prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_004, TestSize.Level0)
{
    std::ofstream testFile("/data/local/tmp/test.abc");
    testFile.close();

    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath("/data/local/tmp/test.abc");

    EXPECT_FALSE(result);
    unlink("/data/local/tmp/test.abc");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_005
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with empty path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath("");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_006
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with non-existent path (RealPath failure)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(
        "/data/app/el1/public/aot_compiler/ark_cache/nonexistent/file.abc");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_001
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with standalone ".."
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("..");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_002
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with multiple ".." traversals
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/../../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_003
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with ".." in filename (not traversal)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/app..name/file");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_004
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with "../" at beginning
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_005
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with ".." at end
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/app/..");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_006
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with Windows-style ".." at beginning
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("..\\etc\\passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_007
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with single dot "." (current directory)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_007, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/./file");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_008
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with empty path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_008, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_001
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with valid matching paths
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("/data/cache/file", "/data/cache/file.an");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_002
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with wrong suffix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("/data/cache/file", "/data/cache/file.txt");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_003
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with different base paths
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("/data/path1/file", "/data/path2/file.an");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_004
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with missing .an suffix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("/data/cache/file", "/data/cache/file");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_005
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with empty strings
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("", ".an");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_001
 * @tc.desc: Test AotArgsVerify::CheckArkCacheDirectoryPrefix with valid path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_001, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::CheckArkCacheDirectoryPrefix(
        "/data/app/el1/public/aot_compiler/ark_cache/test/file", "test");

    EXPECT_TRUE(result);

    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_002
 * @tc.desc: Test AotArgsVerify::CheckArkCacheDirectoryPrefix with invalid format (no slash)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheDirectoryPrefix("invalid_format_file", "test");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_003
 * @tc.desc: Test AotArgsVerify::CheckArkCacheDirectoryPrefix with wrong directory prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheDirectoryPrefix(
        "/data/app/el1/public/wrong_location/test/file", "test");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_004
 * @tc.desc: Test AotArgsVerify::CheckArkCacheDirectoryPrefix with bundle name mismatch
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheDirectoryPrefix(
        "/data/app/el1/public/aot_compiler/ark_cache/wrong_bundle/file", "test_bundle");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkAnFile_001
 * @tc.desc: Test AotArgsVerify::CheckFrameworkAnFile with valid framework path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkAnFile_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckFrameworkAnFile(
        "/data/service/el1/public/for-all-app/framework_ark_cache/test.abc.an");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkAnFile_002
 * @tc.desc: Test AotArgsVerify::CheckFrameworkAnFile with path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkAnFile_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckFrameworkAnFile(
        "/data/service/el1/public/for-all-app/framework_ark_cache/../test.abc.an");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkAnFile_003
 * @tc.desc: Test AotArgsVerify::CheckFrameworkAnFile with wrong path prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkAnFile_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckFrameworkAnFile(
        "/data/app/el1/public/aot_compiler/ark_cache/test.abc.an");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkAnFile_004
 * @tc.desc: Test AotArgsVerify::CheckFrameworkAnFile with Windows-style path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkAnFile_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckFrameworkAnFile(
        "/data/service/el1/public/for-all-app/framework_ark_cache\\..\\test.abc.an");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_001
 * @tc.desc: Test CheckAbcName with empty abcName for AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("", AotParserType::DYNAMIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_002
 * @tc.desc: Test CheckAbcName with wrong abcName for AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/modules_static.abc", AotParserType::DYNAMIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_003
 * @tc.desc: Test CheckAbcName with custom abcName for AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/custom.abc", AotParserType::DYNAMIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_004
 * @tc.desc: Test CheckAbcName with wrong abcName for STATIC_AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/modules.abc", AotParserType::STATIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_005
 * @tc.desc: Test CheckAbcName with empty abcName for STATIC_AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("", AotParserType::STATIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_006
 * @tc.desc: Test CheckAbcName with valid abcName for AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/modules.abc", AotParserType::DYNAMIC_AOT);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_007
 * @tc.desc: Test CheckAbcName with valid abcName for STATIC_AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_007, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/modules_static.abc", AotParserType::STATIC_AOT);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckModuleName_001
 * @tc.desc: Test CheckModuleName with empty moduleName (should pass)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckModuleName_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckModuleName("");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckModuleName_002
 * @tc.desc: Test CheckModuleName with valid moduleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckModuleName_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckModuleName("com.example.module");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckModuleName_003
 * @tc.desc: Test CheckModuleName with path traversal attack
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckModuleName_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckModuleName("../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckModuleName_004
 * @tc.desc: Test CheckModuleName with ".." in middle of name
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckModuleName_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckModuleName("module..name");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcOffsetAndSize_001
 * @tc.desc: Test CheckAbcOffsetAndSize with non-existent HAP file
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcOffsetAndSize_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcOffsetAndSize(0, 1000, "/nonexistent/path/test.hap");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcOffsetAndSize_002
 * @tc.desc: Test CheckAbcOffsetAndSize with offset exceeding file size
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcOffsetAndSize_002, TestSize.Level0)
{
    mkdir("/data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::string testPath = "/data/test/test_hap_002.hap";
    std::ofstream testFile(testPath, std::ios::binary);
    ASSERT_TRUE(testFile.is_open()) << "Failed to create test file: " << testPath;
    testFile << "test content";
    testFile.flush();
    testFile.close();

    bool result = AotArgsVerify::CheckAbcOffsetAndSize(1000, 100, testPath);

    EXPECT_FALSE(result);

    unlink(testPath.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcOffsetAndSize_003
 * @tc.desc: Test CheckAbcOffsetAndSize with offset+size exceeding file size
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcOffsetAndSize_003, TestSize.Level0)
{
    mkdir("/data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::string testPath = "/data/test/test_hap_003.hap";
    std::ofstream testFile(testPath, std::ios::binary);
    ASSERT_TRUE(testFile.is_open()) << "Failed to create test file: " << testPath;
    testFile << "test content";
    testFile.flush();
    testFile.close();

    bool result = AotArgsVerify::CheckAbcOffsetAndSize(10, 10, testPath);

    EXPECT_FALSE(result);

    unlink(testPath.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcOffsetAndSize_004
 * @tc.desc: Test CheckAbcOffsetAndSize with valid offset and size
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcOffsetAndSize_004, TestSize.Level0)
{
    mkdir("/data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::string testPath = "/data/test/test_hap_004.hap";
    std::ofstream testFile(testPath, std::ios::binary);
    ASSERT_TRUE(testFile.is_open()) << "Failed to create test file: " << testPath;
    testFile << "test content";
    testFile.flush();
    testFile.close();

    bool result = AotArgsVerify::CheckAbcOffsetAndSize(5, 5, testPath);

    EXPECT_TRUE(result);

    unlink(testPath.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_001
 * @tc.desc: Test CheckBundleUidAndGid with UID below minimum
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(100, 10000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_002
 * @tc.desc: Test CheckBundleUidAndGid with GID below minimum
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(10000, 100);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_003
 * @tc.desc: Test CheckBundleUidAndGid with valid app UID/GID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(10000, 10000);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_001
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with valid UID/GID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_002
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with missing BUNDLE_UID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_003
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with missing BUNDLE_GID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_003, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_004
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with UID below minimum
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_004, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "9999";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_005
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with GID below minimum
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_005, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "9999";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_006
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with both UID and GID below minimum
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_006, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "100";
    argsMap[ArgsIdx::BUNDLE_GID] = "200";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_007
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with large valid UID/GID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_007, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "99999";
    argsMap[ArgsIdx::BUNDLE_GID] = "99999";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_008
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with invalid UID format
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_008, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "invalid_uid";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_009
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with invalid GID format
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_009, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "invalid_gid";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_010
 * @tc.desc: Test CheckBundleUidAndGidFromArgsMap with both UID and GID invalid format
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGidFromArgsMap_010, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::BUNDLE_UID] = "not_a_number";
    argsMap[ArgsIdx::BUNDLE_GID] = "also_not_a_number";

    bool result = AotArgsVerify::CheckBundleUidAndGidFromArgsMap(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPkgInfoFields_001
 * @tc.desc: Test CheckPkgInfoFields with valid pkgInfo
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPkgInfoFields_001, TestSize.Level0)
{
    mkdir("/data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::string testPath = "/data/test/test.hap";
    std::ofstream hapFile(testPath, std::ios::binary);
    ASSERT_TRUE(hapFile.is_open()) << "Failed to create test file: " << testPath;
    hapFile << "test content";
    hapFile.flush();
    hapFile.close();

    AotPkgInfo pkgInfo;
    pkgInfo.bundleName = "com.example.app";
    pkgInfo.pkgPath = testPath;
    pkgInfo.abcName = "ets/modules.abc";
    pkgInfo.moduleName = "module";
    pkgInfo.abcOffset = 0;
    pkgInfo.abcSize = 10;

    bool result = AotArgsVerify::CheckPkgInfoFields(pkgInfo, AotParserType::DYNAMIC_AOT);

    EXPECT_TRUE(result);
    unlink(testPath.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPkgInfoFields_002
 * @tc.desc: Test CheckPkgInfoFields with invalid moduleName (path traversal)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPkgInfoFields_002, TestSize.Level0)
{
    AotPkgInfo pkgInfo;
    pkgInfo.bundleName = "com.example.app";
    pkgInfo.pkgPath = "/tmp/test.hap";
    pkgInfo.moduleName = "../etc/passwd";
    pkgInfo.abcOffset = 0;
    pkgInfo.abcSize = 100;

    bool result = AotArgsVerify::CheckPkgInfoFields(pkgInfo, AotParserType::DYNAMIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseStringField_001
 * @tc.desc: Test ParseStringField with valid string field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseStringField_001, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", "com.example.app"}};
    std::string output;

    bool result = AotArgsVerify::ParseStringField(jsonObj, "bundleName", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, "com.example.app");
}

/**
 * @tc.name: AotArgsVerifyTest_ParseStringField_002
 * @tc.desc: Test ParseStringField with missing field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseStringField_002, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", "com.example.app"}};
    std::string output;

    bool result = AotArgsVerify::ParseStringField(jsonObj, "pkgPath", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseStringField_003
 * @tc.desc: Test ParseStringField with null field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseStringField_003, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", nullptr}};
    std::string output;

    bool result = AotArgsVerify::ParseStringField(jsonObj, "bundleName", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseStringField_004
 * @tc.desc: Test ParseStringField with non-string field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseStringField_004, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", 12345}};
    std::string output;

    bool result = AotArgsVerify::ParseStringField(jsonObj, "bundleName", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_001
 * @tc.desc: Test ParseUint32Field with valid number
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_001, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"abcOffset", 1000U}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "abcOffset", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 1000U);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_002
 * @tc.desc: Test ParseUint32Field with valid hexadecimal string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_002, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"abcOffset", "3e8"}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "abcOffset", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 1000U);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_003
 * @tc.desc: Test ParseUint32Field with invalid hex string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_003, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"abcOffset", "invalid"}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "abcOffset", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_004
 * @tc.desc: Test ParseUint32Field with missing field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_004, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", "test"}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "abcOffset", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_001
 * @tc.desc: Test ParseInt32Field with valid number
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_001, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", 10000}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 10000);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_002
 * @tc.desc: Test ParseInt32Field with valid hexadecimal string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_002, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", "2710"}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 10000);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_003
 * @tc.desc: Test ParseInt32Field with invalid string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_003, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", "invalid"}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_001
 * @tc.desc: Test ParseAotPkgInfo with valid minimal JSON (all required fields)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_001, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "bundleName": "com.example.app",
        "pkgPath": "/data/test/test.hap",
        "abcName": "ets/modules.abc",
        "moduleName": "module",
        "appIdentifier": "signature",
        "pgoDir": "/data/pgo",
        "abcOffset": "0",
        "abcSize": "64"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_TRUE(result);
    EXPECT_EQ(info.bundleName, "com.example.app");
    EXPECT_EQ(info.pkgPath, "/data/test/test.hap");
    EXPECT_EQ(info.abcName, "ets/modules.abc");
    EXPECT_EQ(info.moduleName, "module");
    EXPECT_EQ(info.appIdentifier, "signature");
    EXPECT_EQ(info.pgoDir, "/data/pgo");
    EXPECT_EQ(info.abcOffset, 0U);
    EXPECT_EQ(info.abcSize, 100U);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_002
 * @tc.desc: Test ParseAotPkgInfo with all fields
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_002, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "bundleName": "com.example.app",
        "pkgPath": "/data/test/test.hap",
        "abcName": "ets/modules.abc",
        "moduleName": "module",
        "appIdentifier": "signature",
        "pgoDir": "/data/pgo",
        "abcOffset": "64",
        "abcSize": "1f4"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_TRUE(result);
    EXPECT_EQ(info.bundleName, "com.example.app");
    EXPECT_EQ(info.pkgPath, "/data/test/test.hap");
    EXPECT_EQ(info.abcName, "ets/modules.abc");
    EXPECT_EQ(info.moduleName, "module");
    EXPECT_EQ(info.appIdentifier, "signature");
    EXPECT_EQ(info.pgoDir, "/data/pgo");
    EXPECT_EQ(info.abcOffset, 100U);
    EXPECT_EQ(info.abcSize, 500U);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_003
 * @tc.desc: Test ParseAotPkgInfo with invalid JSON
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_003, TestSize.Level0)
{
    std::string pkgInfoStr = "{invalid json}";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_004
 * @tc.desc: Test ParseAotPkgInfo with missing required field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_004, TestSize.Level0)
{
    std::string pkgInfoStr = R"({"bundleName": "com.example.app"})";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_005
 * @tc.desc: Test ParseAotPkgInfo with empty JSON object
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_005, TestSize.Level0)
{
    std::string pkgInfoStr = "{}";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_MissingAbcOffset
 * @tc.desc: Test ParseAotPkgInfo with missing abcOffset
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_MissingAbcOffset, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "bundleName": "com.example.app",
        "pkgPath": "/data/test/test.hap",
        "abcName": "ets/modules.abc",
        "moduleName": "module",
        "appIdentifier": "signature",
        "pgoDir": "/data/pgo",
        "abcSize": "64"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_MissingAbcSize
 * @tc.desc: Test ParseAotPkgInfo with missing abcSize
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_MissingAbcSize, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "bundleName": "com.example.app",
        "pkgPath": "/data/test/test.hap",
        "abcName": "ets/modules.abc",
        "moduleName": "module",
        "appIdentifier": "signature",
        "pgoDir": "/data/pgo",
        "abcOffset": "0"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_MissingBundleName
 * @tc.desc: Test ParseAotPkgInfo with missing bundleName field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_MissingBundleName, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "pkgPath": "/data/test/test.hap",
        "abcName": "ets/modules.abc",
        "moduleName": "module",
        "appIdentifier": "signature",
        "pgoDir": "/data/pgo",
        "abcOffset": "0",
        "abcSize": "64"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_MissingAbcName
 * @tc.desc: Test ParseAotPkgInfo with missing abcName field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_MissingAbcName, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "bundleName": "com.example.app",
        "pkgPath": "/data/test/test.hap",
        "moduleName": "module",
        "appIdentifier": "signature",
        "pgoDir": "/data/pgo",
        "abcOffset": "0",
        "abcSize": "64"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_MissingModuleName
 * @tc.desc: Test ParseAotPkgInfo with missing moduleName field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_MissingModuleName, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "bundleName": "com.example.app",
        "pkgPath": "/data/test/test.hap",
        "abcName": "ets/modules.abc",
        "appIdentifier": "signature",
        "pgoDir": "/data/pgo",
        "abcOffset": "0",
        "abcSize": "64"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_MissingAppIdentifier
 * @tc.desc: Test ParseAotPkgInfo with missing appIdentifier field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_MissingAppIdentifier, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "bundleName": "com.example.app",
        "pkgPath": "/data/test/test.hap",
        "abcName": "ets/modules.abc",
        "moduleName": "module",
        "pgoDir": "/data/pgo",
        "abcOffset": "0",
        "abcSize": "64"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseAotPkgInfo_MissingPgoDir
 * @tc.desc: Test ParseAotPkgInfo with missing pgoDir field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseAotPkgInfo_MissingPgoDir, TestSize.Level0)
{
    std::string pkgInfoStr = R"({
        "bundleName": "com.example.app",
        "pkgPath": "/data/test/test.hap",
        "abcName": "ets/modules.abc",
        "moduleName": "module",
        "appIdentifier": "signature",
        "abcOffset": "0",
        "abcSize": "64"
    })";
    AotPkgInfo info;

    bool result = AotArgsVerify::ParseAotPkgInfo(pkgInfoStr, info);

    EXPECT_FALSE(result);
}
// CheckAOTArgs - Return False Branches

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_ParsePkgInfoFail
 * @tc.desc: Test CheckAOTArgs when ParseAotPkgInfo fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_ParsePkgInfoFail, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{invalid_json}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_AnFileNameMismatch
 * @tc.desc: Test CheckAOTArgs when anFileName does not match aotFile + ".an"
 *           Covers branch: CheckArkCacheFiles validates anFile == aotFile + ".an"
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_AnFileNameMismatch, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();
    std::ofstream("/data/app/el1/public/aot_compiler/ark_cache/test/test.an").close();
    std::ofstream("/data/app/el1/public/aot_compiler/ark_cache/test/wrong.an").close();

    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"test\",\"pkgPath\":\"/data/test.hap\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test/wrong.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test/test.an");
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test/wrong.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache");
    unlink("/data/test.hap");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_BundleUidInvalid
 * @tc.desc: Test CheckAOTArgs when CheckBundleUidAndGidFromArgsMap fails due to invalid uid
 *           All previous checks pass, only CheckBundleUidAndGidFromArgsMap fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_BundleUidInvalid, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.test.app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();

    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test.app\",\"pkgPath\":\"/data/test.hap\","
         "\"abcName\":\"ets/modules.abc\",\"moduleName\":\"module\","
         "\"appIdentifier\":\"sig\",\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/com.test.app\","
         "\"abcOffset\":\"0\",\"abcSize\":\"64\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.test.app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/com.test.app/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "9999";  // Invalid: < MIN_APP_UID_GID (10000)
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
    unlink("/data/test.hap");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.test.app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_BundleGidInvalid
 * @tc.desc: Test CheckAOTArgs when CheckBundleUidAndGidFromArgsMap fails due to invalid gid
 *           All previous checks pass, only CheckBundleUidAndGidFromArgsMap fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_BundleGidInvalid, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.test.app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();

    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test.app\",\"pkgPath\":\"/data/test.hap\","
         "\"abcName\":\"ets/modules.abc\",\"moduleName\":\"module\","
         "\"appIdentifier\":\"sig\",\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/com.test.app\","
         "\"abcOffset\":\"0\",\"abcSize\":\"64\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.test.app/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/com.test.app/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "9999";  // Invalid: < MIN_APP_UID_GID (10000)

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
    unlink("/data/test.hap");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.test.app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_PathTraversalInAotFile
 * @tc.desc: Test CheckAOTArgs when CheckArkCacheFiles fails due to path traversal
 *           All previous checks pass, only CheckArkCacheFiles fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_PathTraversalInAotFile, TestSize.Level0)
{
#if !defined(PANDA_TARGET_OHOS)
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();

    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test.app\",\"pkgPath\":\"/data/test.hap\","
         "\"abcName\":\"ets/modules.abc\",\"moduleName\":\"module\","
         "\"appIdentifier\":\"sig\",\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/com.test.app\","
         "\"abcOffset\":\"0\",\"abcSize\":\"64\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/../etc/test";  // Path traversal
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/../etc/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckAOTArgs(argsMap);

    EXPECT_FALSE(result);
    unlink("/data/test.hap");
#endif
}

// CheckStaticAotArgs - Return False Branches

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_MissingPkgInfo
 * @tc.desc: Test CheckStaticAotArgs when COMPILER_PKG_INFO is missing
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_MissingPkgInfo, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_ParsePkgInfoFail
 * @tc.desc: Test CheckStaticAotArgs when ParseAotPkgInfo fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_ParsePkgInfoFail, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{invalid_json}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_CheckPkgInfoFieldsFail
 * @tc.desc: Test CheckStaticAotArgs when CheckPkgInfoFields fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_CheckPkgInfoFieldsFail, TestSize.Level0)
{
#if defined(PANDA_TARGET_OHOS)
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test\",\"pkgPath\":\"/data/test.hap\","
         "\"abcName\":\"ets/modules_static.abc\",\"moduleName\":\"../etc\","
         "\"appIdentifier\":\"signature\",\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/test\","
         "\"abcOffset\":\"0\",\"abcSize\":\"64\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
#else
    std::unordered_map<std::string, std::string> argsMap;
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();
    std::ofstream("/data/app/el1/public/aot_compiler/ark_cache/test/test.an").close();

    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test\",\"pkgPath\":\"/data/test.hap\","
         "\"abcName\":\"ets/modules_static.abc\",\"moduleName\":\"../etc\","
         "\"appIdentifier\":\"signature\",\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/test\","
         "\"abcOffset\":\"0\",\"abcSize\":\"64\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";
    argsMap[ArgsIdx::BUNDLE_UID] = "10000";
    argsMap[ArgsIdx::BUNDLE_GID] = "10000";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test");
    unlink("/data/test.hap");
#endif
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_CheckArkCacheFilesFail
 * @tc.desc: Test CheckStaticAotArgs when CheckArkCacheFiles fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_CheckArkCacheFilesFail, TestSize.Level0)
{
#if defined(PANDA_TARGET_OHOS)
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test\",\"pkgPath\":\"/data/test.hap\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\"}";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
#else
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();

    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test\",\"pkgPath\":\"/data/test.hap\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\"}";

    bool result = AotArgsVerify::CheckStaticAotArgs(argsMap);

    EXPECT_FALSE(result);
    unlink("/data/test.hap");
#endif
}
// CheckAbcOffsetAndSize - Return False Branches

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcOffsetAndSize_FileNotExist
 * @tc.desc: Test CheckAbcOffsetAndSize when file doesn't exist
 *           Covers: if (stat(pkgPath.c_str(), &fileStat) != 0) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcOffsetAndSize_FileNotExist, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcOffsetAndSize(100, 1000, "/nonexistent/file.hap");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcOffsetAndSize_OffsetExceedsSize
 * @tc.desc: Test CheckAbcOffsetAndSize when abcOffset exceeds HAP size
 *           Covers: if (abcOffset > hapSize) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcOffsetAndSize_OffsetExceedsSize, TestSize.Level0)
{
    mkdir("/tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::string testFile = "/tmp/test_offset.hap";
    std::ofstream file(testFile);
    file << "test";
    file.close();

    bool result = AotArgsVerify::CheckAbcOffsetAndSize(100, 10, testFile);

    EXPECT_FALSE(result);

    unlink(testFile.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcOffsetAndSize_SumExceedsSize
 * @tc.desc: Test CheckAbcOffsetAndSize when abcOffset + abcSize exceeds HAP size
 *           Covers: if (abcOffset + abcSize > hapSize) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcOffsetAndSize_SumExceedsSize, TestSize.Level0)
{
    mkdir("/tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::string testFile = "/tmp/test_sum.hap";
    std::ofstream file(testFile);
    file << "test";
    file.close();

    bool result = AotArgsVerify::CheckAbcOffsetAndSize(2, 10, testFile);

    EXPECT_FALSE(result);

    unlink(testFile.c_str());
}
// ParseUint32Field - Wrong Field Type Tests (Line 466-467)
// ============================================================================

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_WrongType_Boolean
 * @tc.desc: Test ParseUint32Field with boolean value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_unsigned()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_WrongType_Boolean, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"abcOffset", true}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "abcOffset", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_WrongType_Array
 * @tc.desc: Test ParseUint32Field with array value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_unsigned()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_WrongType_Array, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"abcOffset", {1, 2, 3}}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "abcOffset", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_WrongType_Object
 * @tc.desc: Test ParseUint32Field with object value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_unsigned()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_WrongType_Object, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"abcOffset", {{"value", "100"}}}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "abcOffset", output);

    EXPECT_FALSE(result);
}
// ParseInt32Field - Missing Test Cases (Lines 487-491)

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_MissingField
 * @tc.desc: Test ParseInt32Field with missing field
 *           Covers: if (!jsonObj.contains(key) || jsonObj[key].is_null()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_MissingField, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", "test"}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_NullField
 * @tc.desc: Test ParseInt32Field with null field
 *           Covers: if (!jsonObj.contains(key) || jsonObj[key].is_null()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_NullField, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", nullptr}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_WrongType_Boolean
 * @tc.desc: Test ParseInt32Field with boolean value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_WrongType_Boolean, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", true}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_WrongType_Array
 * @tc.desc: Test ParseInt32Field with array value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_WrongType_Array, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", {1, 2, 3}}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_WrongType_Object
 * @tc.desc: Test ParseInt32Field with object value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_WrongType_Object, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", {{"value", "100"}}}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_WrongType_Float
 * @tc.desc: Test ParseInt32Field with float value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_WrongType_Float, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", 123.45}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}
// CheckCodeSignArkCacheFilePath - Symlink Path Traversal (Lines 319-321)

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_SymlinkTraversal
 * @tc.desc: Test CheckCodeSignArkCacheFilePath with symlink containing path traversal
 *           Covers: Re-check for path traversal after RealPath (symlink could bypass)
 *           if (!CheckPathTraverse(resolvedPath)) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_SymlinkTraversal, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.example.app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream realFile("/data/app/el1/public/aot_compiler/ark_cache/com.example.app/test.abc");
    realFile << "test content";
    realFile.close();
    std::string symlinkPath = "/data/app/el1/public/aot_compiler/ark_cache/com.example.app/symlink.abc";
    mkdir("/data/local/tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream outsideFile("/data/local/tmp/outside.abc");
    outsideFile << "outside content";
    outsideFile.close();
    symlink("/data/local/tmp/outside.abc", symlinkPath.c_str());
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(symlinkPath);

    EXPECT_FALSE(result);
    unlink(symlinkPath.c_str());
    unlink("/data/local/tmp/outside.abc");
    unlink("/data/app/el1/public/aot_compiler/ark_cache/com.example.app/test.abc");
    rmdir("/data/app/el1/public/aot_compiler/az_cache/com.example.app");
}

}
